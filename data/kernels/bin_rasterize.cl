
#include "oclr_global.h"
#include "oclr_math.h"

typedef struct __attribute__((packed, aligned(16))) {
	unsigned int triangle_count;
} constant_data;

typedef struct __attribute__((packed, aligned(16))) {
	// VV0: 0 - 2
	// VV1: 3 - 5
	// VV2: 6 - 8
	// depth: 9
	// unused: 10 - 11
	// x_bounds: 12 - 13 (.x/12 = INFINITY if culled)
	// y_bounds: 14 - 15
	float data[16];
} transformed_data;

//
// TODO: compile time define
#define BIN_SIZE 64u
#define BATCH_SIZE 256u
kernel void bin_rasterize(global unsigned int* bin_distribution_counter,
						  global ulong* bin_queues,
						  const uint2 bin_count,
						  const unsigned int bin_count_lin,
						  const unsigned int batch_count,
						  const unsigned int batch_size,
						  const unsigned int triangle_count,
						  
						  global const transformed_data* transformed_buffer,
						  const uint2 framebuffer_size
						  ) {
	const unsigned int local_id = get_local_id(0);
	//const unsigned int local_size = get_local_size(0);
	const uint2 bin_location = (uint2)(local_id % bin_count.x, local_id / bin_count.x);
	
	// -> each work-item: 1 bin + private mem queue
	// -> iterate over 256 triangles (batch size: 256)
	// -> store loop index in priv mem queue (-> only one byte per triangle)
	// -> 2 vstore16 calls with 2 ulong16s (256 bytes)
	
	// note: opencl does not require this to be aligned, but certain implementations do
	uchar triangle_queue[BATCH_SIZE] __attribute__((aligned(8)));
	unsigned int triangles_in_queue = 0;
	
#define ASYNC_COPY 1
#if defined(ASYNC_COPY)
	local float4 triangle_bounds[BATCH_SIZE] __attribute__((aligned(16))); // correctly align, so async copy will work
#endif
	local unsigned int batch_idx;
	const uint2 framebuffer_clamp_size = framebuffer_size - 1u;
	for(;;) {
		// get next batch index
		if(local_id == 0) {
			// only done once per work-group (-> only work-item #0)
			batch_idx = atomic_inc(bin_distribution_counter);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// check if all batches have been processed
		if(batch_idx >= batch_count) {
			//printf("[%u] %u\n", local_id, triangles_in_queue);
			return;
		}
		
		// read input triangle bounds into shared memory (across work-group)
		const unsigned int triangle_id_offset = batch_idx * batch_size;
#if defined(ASYNC_COPY)
		event_t event = async_work_group_strided_copy(&triangle_bounds[0],
													  (global const float4*)&transformed_buffer[triangle_id_offset].data[12],
													  BATCH_SIZE, 4, 0);
		wait_group_events(1, &event);
#endif
		
		for(unsigned int triangle_id = triangle_id_offset, idx = 0,
			last_triangle_id = min(triangle_id + batch_size, triangle_count);
			triangle_id < last_triangle_id; triangle_id++, idx++) {
			//printf("\ttri: %u\n", triangle_id);
			
#if defined(ASYNC_COPY)
			// cull:
			if(triangle_bounds[idx].x == INFINITY) continue;
			
			// TODO: read triangle data into local mem across work-items (less global mem reads)
			const uint2 x_bounds = (uint2)(convert_uint(triangle_bounds[idx].x),
										   convert_uint(triangle_bounds[idx].y));
			const uint2 y_bounds = (uint2)(convert_uint(triangle_bounds[idx].z),
										   convert_uint(triangle_bounds[idx].w));
#else
			// cull:
			if(transformed_buffer[triangle_id_offset].data[12] == INFINITY) continue;
			
			// TODO: read triangle data into local mem across work-items (less global mem reads)
			const uint2 x_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id_offset].data[12]),
										   convert_uint(transformed_buffer[triangle_id_offset].data[13]));
			const uint2 y_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id_offset].data[14]),
										   convert_uint(transformed_buffer[triangle_id_offset].data[15]));
#endif
			
			// valid pixel pos: [0, framebuffer_size - 1]
			const uint2 x_bounds_u = (uint2)(clamp(x_bounds.x, 0u, framebuffer_clamp_size.x),
											 clamp(x_bounds.y, 0u, framebuffer_clamp_size.x));
			const uint2 y_bounds_u = (uint2)(clamp(y_bounds.x, 0u, framebuffer_clamp_size.y),
											 clamp(y_bounds.y, 0u, framebuffer_clamp_size.y));
			
			// TODO: already read depth from framebuffer in here -> cull if greater depth
			
			// insert triangle id intro appropriate queues/bins
			const uint2 x_bins = x_bounds_u / BIN_SIZE;
			const uint2 y_bins = y_bounds_u / BIN_SIZE;
			
			if(bin_location.y >= y_bins.x && bin_location.y <= y_bins.y &&
			   bin_location.x >= x_bins.x && bin_location.x <= x_bins.y) {
				triangle_queue[triangles_in_queue] = idx;
				triangles_in_queue++;
			}
		}
		
		if(triangles_in_queue > 0) {
			const ulong16* __attribute__((aligned(8))) queue_data_ptr = (const ulong16*)&triangle_queue[0];
			const size_t offset = ((bin_count.x * bin_count.y) * batch_idx + local_id) * (BATCH_SIZE / 128u);
			vstore16(*queue_data_ptr, offset, bin_queues);
			if(triangles_in_queue > 128u) {
				// only necessary, if there are more than 128 triangles in the bin queue
				vstore16(*(queue_data_ptr+1), offset+1, bin_queues);
			}
		}
		// TODO: store triangle count
	}
}

/*kernel void coarse_rasterize(global const transformed_data* transformed_buffer,
							 global unsigned int* triangle_queues_buffer,
							 global unsigned int* queue_sizes_buffer,
							 const uint2 screen_size,
							 const uint2 tile_size,
							 const uint2 bin_count,
							 const unsigned int triangles_per_group,
							 const unsigned int triangle_count) {
	// TODO
	return;
}*/
