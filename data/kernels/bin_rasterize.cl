
#include "oclr_global.h"
#include "oclr_math.h"

typedef struct __attribute__((packed, aligned(16))) {
	unsigned int triangle_count;
} constant_data;

typedef struct __attribute__((packed, aligned(16))) {
	// VV0: 0 - 2
	// VV1: 3 - 5
	// VV2: 6 - 8
	// depth: 9 (INFINITY if culled)
	// x_bounds: 10 - 11
	// y_bounds: 12 - 13
	// unused: 14 - 15
	float data[16];
} transformed_data;

//
// TODO: compile time define
#define BIN_SIZE 128u
kernel void bin_rasterize(global unsigned int* bin_distribution_counter,
						  const unsigned int batch_count,
						  const unsigned int batch_size,
						  const unsigned int triangle_count,
						  
						  global const transformed_data* transformed_buffer,
						  const uint2 framebuffer_size
						  ) {
	const unsigned int local_id = get_local_id(0);
	const unsigned int local_size = get_local_size(0);
	
	local unsigned int batch_idx;
	for(;;) {
		// get next batch index
		if(local_id == 0) {
			// only done once per work-group (-> only work-item #0)
			batch_idx = atomic_inc(bin_distribution_counter);
		}
		barrier(CLK_LOCAL_MEM_FENCE);
		
		// check if all batches have been processed
		if(batch_idx >= batch_count) return;
		
		//const unsigned int tri_idx = batch_idx * batch_size + local_id;
		for(unsigned int tri_idx = local_id; tri_idx < batch_size; tri_idx += local_size) {
			const unsigned int triangle_id = batch_idx * batch_size + tri_idx;
			//printf("\ttri: %u\n", triangle_id);
			if(triangle_id >= triangle_count) return;
			
			// cull:
			//if(transformed_buffer[triangle_id].data[9] == INFINITY) continue;
			
			const float2 x_bounds = (float2)(transformed_buffer[triangle_id].data[10], transformed_buffer[triangle_id].data[11]);
			const float2 y_bounds = (float2)(transformed_buffer[triangle_id].data[12], transformed_buffer[triangle_id].data[13]);
			
			// TODO: rounding should depend on sampling mode (more samples -> use floor/ceil again)
#if !defined(APPLE_ARM) && 0
			const uint2 x_bounds_u = convert_uint2(clamp((int2)(round(x_bounds.x), round(x_bounds.y)),
														 0, framebuffer_size.x - 1)); // valid pixel pos: [0, screen_size.x-1]
			const uint2 y_bounds_u = convert_uint2(clamp((int2)(round(y_bounds.x), round(y_bounds.y)),
														 0, framebuffer_size.y - 1));
#else
			// valid pixel pos: [0, screen_size.x-1]
			const uint2 x_bounds_u = convert_uint2((int2)(clamp((int)round(x_bounds.x), 0, (int)framebuffer_size.x - 1),
														  clamp((int)round(x_bounds.y), 0, (int)framebuffer_size.x - 1)));
			const uint2 y_bounds_u = convert_uint2((int2)(clamp((int)round(y_bounds.x), 0, (int)framebuffer_size.y - 1),
														  clamp((int)round(y_bounds.y), 0, (int)framebuffer_size.y - 1)));
#endif
			
			// TODO: already read depth from framebuffer in here -> cull if greater depth
			
			// insert triangle id intro appropriate queues/bins
			const uint2 x_bins = x_bounds_u / BIN_SIZE;
			const uint2 y_bins = y_bounds_u / BIN_SIZE;
			const unsigned int queue_size = triangle_count; // TODO: only true for now
			
			for(unsigned int y_bin = y_bins.x; y_bin <= y_bins.y; y_bin++) {
				for(unsigned int x_bin = x_bins.x; x_bin <= x_bins.y; x_bin++) {
					//const unsigned int bin_index = y_bin * bin_count.x + x_bin;
					//const unsigned int queue_index = atomic_inc(&queue_sizes_buffer[bin_index]);
					//const unsigned int queue_offset = (queue_size * bin_index) + queue_index;
					//triangle_queues_buffer[queue_offset] = triangle_id;
				}
			}
		}
		
		//
		//printf("batch idx: %u\n", batch_idx);
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
