
#include "oclr_global.h"
#include "oclr_math.h"

typedef struct __attribute__((packed, aligned(16))) {
	float4 bounds; // (.x = INFINITY if culled)
} primitive_bounds;

//
kernel void oclraster_bin(global unsigned int* bin_distribution_counter,
						  global ulong* bin_queues,
						  const uint2 bin_count,
						  const unsigned int bin_count_lin,
						  const uint2 bin_offset,
						  const unsigned int batch_count,
						  const unsigned int primitive_count,
						  
						  global const primitive_bounds* primitive_bounds_buffer,
						  const uint2 framebuffer_size
#if !defined(CPU)
						  , const unsigned int intra_bin_groups
#endif
						  ) {
	const unsigned int local_id = get_local_id(0);
	const unsigned int local_size = get_local_size(0);
	
	// TODO: already read depth from framebuffer in here -> cull if depth test fails
	
	// -> each work-item: 1 bin + private mem queue (gpu version) or 1 batch + private mem queue (cpu version)
	// -> iterate over 255 primitives (batch size: 255)
	// -> store loop index in priv mem queue (-> only one byte per primitive)
	// -> 1 vstore4 calls with 1 ulong4 (32 bytes)
	
	// queue storage handling
	ulong4 primitive_queue_vec;
	uchar* primitive_queue = (uchar*)&primitive_queue_vec;
	
	// framebuffer range is [0, size - 1], clamp accordingly
	const uint2 framebuffer_clamp_size = framebuffer_size - 1u;
	
#if !defined(CPU)
	// GPU version
	
	const unsigned int global_id = get_global_id(0);
	
	// init counter
	if(global_id == 0) {
		*bin_distribution_counter = 0;
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
	
	// note: opencl does not require this to be aligned, but certain implementations do
	local float4 primitive_bounds[BATCH_PRIMITIVE_COUNT] __attribute__((aligned(16))); // correctly align, so async copy will work
	local unsigned int batch_idx;
	
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
		
		// read input primitive bounds into shared memory (across work-group)
		const unsigned int primitive_id_offset = batch_idx * BATCH_PRIMITIVE_COUNT;
		event_t event = async_work_group_copy(&primitive_bounds[0],
											  (global const float4*)&primitive_bounds_buffer[primitive_id_offset],
											  BATCH_PRIMITIVE_COUNT, 0);
		wait_group_events(1, &event);
		
		// in cases where #bins > #work-items, we need to iterate over all bins (simply offset the bin_idx by the #work-items)
		for(unsigned int bin_idx_offset = 0; bin_idx_offset < intra_bin_groups; bin_idx_offset++) {
			const unsigned int bin_idx = local_id + (bin_idx_offset * local_size);
			if(bin_idx >= bin_count_lin) break;
			const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x) + bin_offset;
			
			// iterate over all primitives in this batch
			unsigned int primitives_in_queue = 0;
			primitive_queue_vec = (ulong4)(0ULL, 0ULL, 0ULL, 0ULL); // init all primitive bytes to 0 (-> all invisible)
			
			for(unsigned int primitive_id = primitive_id_offset, primitive_counter = 0u,
				last_primitive_id = min(primitive_id_offset + BATCH_PRIMITIVE_COUNT, primitive_count);
				primitive_id < last_primitive_id;
				primitive_id++, primitive_counter++) {
				// cull:
				if(primitive_bounds[primitive_counter].x == INFINITY) continue;
				
				const uint2 x_bounds = (uint2)(convert_uint(primitive_bounds[primitive_counter].x),
											   convert_uint(primitive_bounds[primitive_counter].y));
				const uint2 y_bounds = (uint2)(convert_uint(primitive_bounds[primitive_counter].z),
											   convert_uint(primitive_bounds[primitive_counter].w));
#else
	// CPU version (no barriers, no group-waiting, no local-mem)
	
	const unsigned int bin_idx = get_group_id(0);
	const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x) + bin_offset;
	{
		for(unsigned int batch_idx = local_id; batch_idx < batch_count; batch_idx += local_size) {
			unsigned int primitives_in_queue = 0;
			primitive_queue_vec = (ulong4)(0ULL, 0ULL, 0ULL, 0ULL); // init all primitive bytes to 0 (-> all invisible)
			const unsigned int primitive_id_offset = batch_idx * BATCH_PRIMITIVE_COUNT;
			
			for(unsigned int primitive_id = primitive_id_offset, primitive_counter = 0u,
				last_primitive_id = min(primitive_id_offset + BATCH_PRIMITIVE_COUNT, primitive_count);
				primitive_id < last_primitive_id;
				primitive_id++, primitive_counter++) {
				// cull:
				if(primitive_bounds_buffer[primitive_id].bounds.x == INFINITY) continue;
				
				const uint2 x_bounds = (uint2)(convert_uint(primitive_bounds_buffer[primitive_id].bounds.x),
											   convert_uint(primitive_bounds_buffer[primitive_id].bounds.y));
				const uint2 y_bounds = (uint2)(convert_uint(primitive_bounds_buffer[primitive_id].bounds.z),
											   convert_uint(primitive_bounds_buffer[primitive_id].bounds.w));
			
#endif
				// valid pixel pos: [0, framebuffer_size - 1]
				const uint2 x_bounds_u = (uint2)(clamp(x_bounds.x, 0u, framebuffer_clamp_size.x),
												 clamp(x_bounds.y, 0u, framebuffer_clamp_size.x));
				const uint2 y_bounds_u = (uint2)(clamp(y_bounds.x, 0u, framebuffer_clamp_size.y),
												 clamp(y_bounds.y, 0u, framebuffer_clamp_size.y));
				
				// insert primitive id intro appropriate queues/bins
				const uint2 x_bins = x_bounds_u / BIN_SIZE;
				const uint2 y_bins = y_bounds_u / BIN_SIZE;
				
				if(bin_location.y >= y_bins.x && bin_location.y <= y_bins.y &&
				   bin_location.x >= x_bins.x && bin_location.x <= x_bins.y) {
					const unsigned int queue_bit = (primitive_counter + 1u) % 8u, queue_byte = ((primitive_counter + 1u) / 8u);
					primitive_queue[queue_byte] |= (1u << queue_bit);
					primitives_in_queue++;
				}
			}
			
			// store the "any primitives visible at all" flag in the first bit of the first byte
			primitive_queue[0] |= (primitives_in_queue > 0u ? 1u : 0u);
	
			// copy queue to global memory (note that some/all implementations have 64-bit loads/stores -> use an ulong4)
#if (BATCH_SIZE == 256u)
			const size_t offset = (bin_idx * batch_count + batch_idx);
			vstore4(primitive_queue_vec, offset, bin_queues);
#else
#error "invalid batch size (only 256 is currently supported)!"
#endif
		}
	}
}
