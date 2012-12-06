
#include "oclr_global.h"
#include "oclr_math.h"

typedef struct __attribute__((packed)) {
	unsigned int triangle_count;
} constant_data;

typedef struct __attribute__((packed)) {
	float4 VV0;
	float4 VV1;
	float4 VV2;
	float4 W;
} transformed_data;

//
kernel void bin_rasterize(global const transformed_data* transformed_buffer,
						  global unsigned int* triangle_queues_buffer,
						  global unsigned int* queue_sizes_buffer,
						  const uint2 screen_size,
						  const uint2 tile_size,
						  const uint2 bin_count,
						  const unsigned int triangles_per_group,
						  const unsigned int triangle_count) {
	const unsigned int triangle_id = get_global_id(0);
	if(triangle_id >= get_global_size(0)) return;
	if(triangle_id >= triangle_count) return;
	
	//const size_t group_id = triangle_id / triangles_per_group;
	//const size_t group_id = get_group_id(0); // TODO: check if this is continuous/order maintaining
	
	const float4 VV0 = transformed_buffer[triangle_id].VV0;
	const float4 VV1 = transformed_buffer[triangle_id].VV1;
	const float4 VV2 = transformed_buffer[triangle_id].VV2;
	
	// compute x/y bounds
	/*print_float4(VV0);
	print_float4(VV1);
	print_float4(VV2);*/
	
	const float x0 = (VV1.y * VV2.z - VV1.z * VV2.y) / (VV1.x * VV2.y - VV1.y * VV2.x);
	const float x1 = (VV0.y * VV2.z - VV0.z * VV2.y) / (VV0.x * VV2.y - VV0.y * VV2.x);
	const float x2 = (VV0.y * VV1.z - VV0.z * VV1.y) / (VV0.x * VV1.y - VV0.y * VV1.x);
	const float y0 = -(x0 * VV1.x + VV1.z) / VV1.y;
	const float y1 = -(x1 * VV2.x + VV2.z) / VV2.y;
	const float y2 = -(x2 * VV0.x + VV0.z) / VV0.y;
	
	const uint2 x_bounds = (uint2)clamp((int2)(floor(min(min(x0, x1), x2)), ceil(max(max(x0, x1), x2))),
										0, screen_size.x - 1); // valid pixel pos: [0, screen_size.x-1]
	const uint2 y_bounds = (uint2)clamp((int2)(floor(min(min(y0, y1), y2)), ceil(max(max(y0, y1), y2))),
										0, screen_size.y - 1);
	
	/*printf("P0: %f %f; P1: %f %f; P2: %f %f\n",
		   x0, y0, x1, y1, x2, y2);*/
	/*printf("bounds: X: [%u, %u], Y: [%u, %u]\n",
		   x_bounds.x, x_bounds.y, y_bounds.x, y_bounds.y);*/
	
	// insert triangle id intro appropriate queues/bins
	const uint2 x_bins = x_bounds / tile_size.x;
	const uint2 y_bins = y_bounds / tile_size.y;
	const unsigned int queue_size = triangle_count; // TODO: only true for now
	for(unsigned int y_bin = y_bins.x; y_bin <= y_bins.y; y_bin++) {
		for(unsigned int x_bin = x_bins.x; x_bin <= x_bins.y; x_bin++) {
			const unsigned int bin_index = y_bin * bin_count.x + x_bin;
			const unsigned int queue_index = atomic_inc(&queue_sizes_buffer[bin_index]);
			const unsigned int queue_offset = (queue_size * bin_index) + queue_index;
			triangle_queues_buffer[queue_offset] = triangle_id;
			//printf("[bin insert: %u %u %u; @%u <- %u]\n",
			//	   queue_size, bin_index, queue_index,
			//	   ((queue_size * bin_index) + queue_index), triangle_id);
		}
	}
}
