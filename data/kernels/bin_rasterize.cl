
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
						  const uint2 bin_count,
						  const unsigned int batch_count,
						  const unsigned int batch_size,
						  const unsigned int triangle_count,
						  
						  global const transformed_data* transformed_buffer,
						  const uint2 framebuffer_size
						  ) {
	const unsigned int local_id = get_local_id(0);
	//const unsigned int local_size = get_local_size(0);
	const uint2 bin_location = (uint2)(local_id % bin_count.x, local_id / bin_count.x);
	
	// TODO: -> read input triangles into shared memory (across work-group)
	
	// -> each work-item: 1 bin + private mem queue
	// -> iterate over 256 triangles (batch size: 256)
	// -> store loop index in priv mem queue (-> only one byte per triangle)
	// -> 2 vstore16 calls with 2 ulong16s (256 bytes)
	uchar triangle_queue[256];
	unsigned int triangles_in_queue = 0;
	
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
		
		for(unsigned int triangle_id = batch_idx * batch_size,
			last_triangle_id = min(triangle_id + batch_size, triangle_count);
			triangle_id < last_triangle_id; triangle_id++) {
			//printf("\ttri: %u\n", triangle_id);
			
			// cull:
			if(transformed_buffer[triangle_id].data[9] == INFINITY) continue;
			
			// TODO: read triangle data into local mem across work-items (less global mem reads)
			const uint2 x_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id].data[10]),
										   convert_uint(transformed_buffer[triangle_id].data[11]));
			const uint2 y_bounds = (uint2)(convert_uint(transformed_buffer[triangle_id].data[12]),
										   convert_uint(transformed_buffer[triangle_id].data[13]));
			
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
				//triangle_queue[triangles_in_queue] = triangle_id;
				triangles_in_queue++;
			}
		}
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
