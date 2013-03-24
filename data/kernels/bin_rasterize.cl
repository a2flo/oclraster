
#include "oclr_global.h"
#include "oclr_math.h"

typedef struct __attribute__((packed, aligned(16))) {
	// VV0: 0 - 2
	// VV1: 3 - 5
	// VV2: 6 - 8
	// depth: 9
	// unused: 10 - 11
	// x_bounds: 12 - 13 (.x/12 = INFINITY if culled)
	// y_bounds: 14 - 15
	const float data[16];
} transformed_data;

//
// TODO: compile time define
#define BIN_SIZE 64u
#define BATCH_SIZE 256u
kernel void oclraster_bin(global unsigned int* bin_distribution_counter,
						  global ulong* bin_queues,
						  const uint2 bin_count,
						  const unsigned int bin_count_lin,
						  const unsigned int batch_count,
						  const unsigned int triangle_count,
						  
						  global const transformed_data* transformed_buffer,
						  const uint2 framebuffer_size
						  ) {
	const unsigned int local_id = get_local_id(0);
	
	// TODO: already read depth from framebuffer in here -> cull if greater depth
	
	// -> each work-item: 1 bin + private mem queue (gpu version) or 1 batch + private mem queue (cpu version)
	// -> iterate over 256 triangles (batch size: 256)
	// -> store loop index in priv mem queue (-> only one byte per triangle)
	// -> 2 vstore16 calls with 2 ulong16s (256 bytes)
	
#if !defined(CPU)
	// GPU version
	
	const unsigned int global_id = get_global_id(0);
	const unsigned int bin_idx = local_id;
	const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x);
	
	// init counter
	if(global_id == 0) {
		*bin_distribution_counter = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	
	// note: opencl does not require this to be aligned, but certain implementations do
	uchar triangle_queue[BATCH_SIZE] __attribute__((aligned(8)));
	local float4 triangle_bounds[BATCH_SIZE] __attribute__((aligned(16))); // correctly align, so async copy will work
	local unsigned int batch_idx;
	const uint2 framebuffer_clamp_size = framebuffer_size - 1u;
	for(;;) {
		// get next batch index
		// note that this barrier is necessary, because not all work-items are running this kernel synchronously
		barrier(CLK_LOCAL_MEM_FENCE);
		if(local_id == 0) {
			// only done once per work-group (-> only work-item #0)
			batch_idx = atomic_inc(bin_distribution_counter);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// check if all batches have been processed
		if(batch_idx >= batch_count) {
			return;
		}
		
		unsigned int triangles_in_queue = 0;
		// read input triangle bounds into shared memory (across work-group)
		const unsigned int triangle_id_offset = batch_idx * BATCH_SIZE;
		event_t event = async_work_group_strided_copy(&triangle_bounds[0],
													  (global const float4*)&transformed_buffer[triangle_id_offset].data[12],
													  BATCH_SIZE, 4, 0);
		wait_group_events(1, &event);
		
		for(unsigned int triangle_id = triangle_id_offset, idx = 0,
			last_triangle_id = min(triangle_id_offset + BATCH_SIZE, triangle_count);
			triangle_id < last_triangle_id; triangle_id++, idx++) {
			// cull:
			if(triangle_bounds[idx].x == INFINITY) continue;
			
			const uint2 x_bounds = (uint2)(convert_uint(triangle_bounds[idx].x),
										   convert_uint(triangle_bounds[idx].y));
			const uint2 y_bounds = (uint2)(convert_uint(triangle_bounds[idx].z),
										   convert_uint(triangle_bounds[idx].w));
#else
	// CPU version (no barriers, no group-waiting, no local-mem)
	
	const unsigned int local_size = get_local_size(0);
	const unsigned int bin_idx = get_group_id(0);
	const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x);
	
	// note: opencl does not require this to be aligned, but certain implementations do
	uchar triangle_queue[BATCH_SIZE] __attribute__((aligned(8)));
	const uint2 framebuffer_clamp_size = framebuffer_size - 1u;
	for(unsigned int batch_idx = local_id; batch_idx < batch_count; batch_idx += local_size) {
		unsigned int triangles_in_queue = 0;
		const unsigned int triangle_id_offset = batch_idx * BATCH_SIZE;
		for(unsigned int triangle_id = triangle_id_offset, idx = 0,
			last_triangle_id = min(triangle_id_offset + BATCH_SIZE, triangle_count);
			triangle_id < last_triangle_id; triangle_id++, idx++) {
			// cull:
			if(transformed_buffer[triangle_id].data[12] == INFINITY) continue;
			
			const uint2 x_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id].data[12]),
										   convert_uint(transformed_buffer[triangle_id].data[13]));
			const uint2 y_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id].data[14]),
										   convert_uint(transformed_buffer[triangle_id].data[15]));
			
#endif
			
			// valid pixel pos: [0, framebuffer_size - 1]
			const uint2 x_bounds_u = (uint2)(clamp(x_bounds.x, 0u, framebuffer_clamp_size.x),
											 clamp(x_bounds.y, 0u, framebuffer_clamp_size.x));
			const uint2 y_bounds_u = (uint2)(clamp(y_bounds.x, 0u, framebuffer_clamp_size.y),
											 clamp(y_bounds.y, 0u, framebuffer_clamp_size.y));
			
			// insert triangle id intro appropriate queues/bins
			const uint2 x_bins = x_bounds_u / BIN_SIZE;
			const uint2 y_bins = y_bounds_u / BIN_SIZE;
			
			if(bin_location.y >= y_bins.x && bin_location.y <= y_bins.y &&
			   bin_location.x >= x_bins.x && bin_location.x <= x_bins.y) {
				triangle_queue[triangles_in_queue] = idx;
				triangles_in_queue++;
			}
		}
		
		if(triangles_in_queue == 0) {
			// flag empty queue with 0xFFFF
			triangle_queue[0] = 0xFF;
			triangle_queue[1] = 0xFF;
		}
		else if(triangles_in_queue != 256) {
			// set end of queue byte (0x00)
			triangle_queue[triangles_in_queue] = 0x00;
			triangles_in_queue++;
		}
		
		// this stores batches for each bin sequentially
		const ulong16* __attribute__((aligned(8))) queue_data_ptr = (const ulong16*)&triangle_queue[0];
		const size_t offset = (bin_idx * batch_count + batch_idx) * (BATCH_SIZE / 128u);
		vstore16(*queue_data_ptr, offset, bin_queues);
		if(triangles_in_queue > 128u) {
			// only necessary, if there are more than 128 triangles in the bin queue
			vstore16(*(queue_data_ptr+1), offset+1, bin_queues);
		}
	}
}
