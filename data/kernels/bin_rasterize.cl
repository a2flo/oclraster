
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
	// cam relation: 10 - 12
	// unused: 13 - 15
	float data[16];
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
	
	const float3 VV[3] = {
		(float3)(transformed_buffer[triangle_id].data[0],
				 transformed_buffer[triangle_id].data[1],
				 transformed_buffer[triangle_id].data[2]),
		(float3)(transformed_buffer[triangle_id].data[3],
				 transformed_buffer[triangle_id].data[4],
				 transformed_buffer[triangle_id].data[5]),
		(float3)(transformed_buffer[triangle_id].data[6],
				 transformed_buffer[triangle_id].data[7],
				 transformed_buffer[triangle_id].data[8])
	};
	
	// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
	const float triangle_cam_relation[3] = {
		transformed_buffer[triangle_id].data[10],
		transformed_buffer[triangle_id].data[11],
		transformed_buffer[triangle_id].data[12]
	};
	
	/*printf("[%d] (%f %f %f) (%f %f %f) (%f %f %f)\n",
		   triangle_id,
		   VV[0].x, VV[0].y, VV[0].z,
		   VV[1].x, VV[1].y, VV[1].z,
		   VV[2].x, VV[2].y, VV[2].z);*/
	unsigned int passing_indices[3] = { 0, 0, 0 };
	float2 clipping_coords[3] = { (float2)(0.0f, 0.0f), (float2)(0.0f, 0.0f), (float2)(0.0f, 0.0f) };
	unsigned int passing_direct = 0;
	unsigned int clipped_count = 0;
	
	// compute x/y bounds
	// valid clip and vertex positions: 0 <= x < screen_size.x && 0 <= y < screen_size.y
	const float fscreen_size[2] = { convert_float(screen_size.x), convert_float(screen_size.y) };
	float2 x_bounds = (float2)(fscreen_size[0], 0.0f);
	float2 y_bounds = (float2)(fscreen_size[1], 0.0f);
	
#define viewport_test(coord, axis) ((coord < 0.0f || coord >= fscreen_size[axis]) ? -1.0f : coord)
#define discard() printf("[%d] -> discard\n", triangle_id); return;
	
	float clipxs[9];
	float clipys[9];
	for(unsigned int i = 0u; i < 3u; i++) {
		// { 1, 2 }, { 0, 2 }, { 0, 1 }
		const unsigned int i0 = (i == 0u ? 1u : 0u);
		const unsigned int i1 = (i == 2u ? 1u : 2u);
		
		const float d = 1.0f / (VV[i0].x * VV[i1].y - VV[i0].y * VV[i1].x);
		clipxs[i] = (triangle_cam_relation[i] < 0.0f ? -1.0f :
					 (VV[i0].y * VV[i1].z - VV[i0].z * VV[i1].y) * d);
		clipys[i] = (triangle_cam_relation[i] < 0.0f ? -1.0f :
					 (VV[i0].z * VV[i1].x - VV[i0].x * VV[i1].z) * d);
		clipxs[i] = viewport_test(clipxs[i], 0);
		clipys[i] = viewport_test(clipys[i], 1);
		
		if(clipxs[i] >= 0.0f && clipys[i] >= 0.0f) {
			//printf("[%d] %d | x: %f, y: %f\n", triangle_id, i, clipxs[i], clipys[i]);
			x_bounds.x = min(x_bounds.x, clipxs[i]);
			x_bounds.y = max(x_bounds.y, clipxs[i]);
			y_bounds.x = min(y_bounds.x, clipys[i]);
			y_bounds.y = max(y_bounds.y, clipys[i]);
			passing_indices[i]++;
			passing_direct++;
			clipping_coords[i] = (float2)(clipxs[i], clipys[i]);
		}
		
		//
		clipxs[i+3] = -VV[i].z / VV[i].x;
		clipys[i+3] = -VV[i].z / VV[i].y;
		clipxs[i+3] = viewport_test(clipxs[i+3], 0);
		clipys[i+3] = viewport_test(clipys[i+3], 1);
		
		clipxs[i+6] = -(VV[i].z + VV[i].y * fscreen_size[1]) / VV[i].x;
		clipys[i+6] = -(VV[i].z + VV[i].x * fscreen_size[0]) / VV[i].y;
		clipxs[i+6] = viewport_test(clipxs[i+6], 0);
		clipys[i+6] = viewport_test(clipys[i+6], 1);
	}
	
	// check remaining clip coordinates
	for(unsigned int i = 0; i < 3; i++) {
		const float cx = clipxs[i + 3];
		const float cy = clipys[i + 3];
		const float cmx = clipxs[i + 6];
		const float cmy = clipys[i + 6];
		
		const unsigned int edge_0 = (i + 1) % 3;
		const unsigned int edge_1 = (i + 2) % 3;
		
		if(cx >= 0.0f) {
			float val1 = cx * VV[edge_0].x + VV[edge_0].z;
			float val2 = cx * VV[edge_1].x + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				//printf("[%d] %d | x: %f\n", triangle_id, i, cx);
				x_bounds.x = min(x_bounds.x, cx);
				x_bounds.y = max(x_bounds.y, cx);
				y_bounds.x = 0.0f;
				passing_indices[i]++;
				clipped_count++;
				clipping_coords[i] = (float2)(cx, 0.0f);
			}
		}
		if(cy >= 0.0f) {
			float val1 = cy * VV[edge_0].y + VV[edge_0].z;
			float val2 = cy * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				//printf("[%d] %d | y: %f\n", triangle_id, i, cy);
				y_bounds.x = min(y_bounds.x, cy);
				y_bounds.y = max(y_bounds.y, cy);
				x_bounds.x = 0.0f;
				passing_indices[i]++;
				clipped_count++;
				clipping_coords[i] = (float2)(0.0f, cy);
			}
		}
		if(cmx >= 0.0f) {
			float val1 = cmx * VV[edge_0].x + fscreen_size[1] * VV[edge_0].y + VV[edge_0].z;
			float val2 = cmx * VV[edge_1].x + fscreen_size[1] * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				//printf("[%d] %d | xm: %f\n", triangle_id, i, cmx);
				x_bounds.x = min(x_bounds.x, cmx);
				x_bounds.y = max(x_bounds.y, cmx);
				y_bounds.y = fscreen_size[1];
				passing_indices[i]++;
				clipped_count++;
				clipping_coords[i] = (float2)(cmx, 0.0f);
			}
		}
		if(cmy >= 0.0f) {
			float val1 = fscreen_size[0] * VV[edge_0].x + cmy * VV[edge_0].y + VV[edge_0].z;
			float val2 = fscreen_size[0] * VV[edge_1].x + cmy * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				//printf("[%d] %d | ym: %f\n", triangle_id, i, cmy);
				y_bounds.x = min(y_bounds.x, cmy);
				y_bounds.y = max(y_bounds.y, cmy);
				x_bounds.y = fscreen_size[0];
				passing_indices[i]++;
				clipped_count++;
				clipping_coords[i] = (float2)(0.0f, cmy);
			}
		}
	}
	
	const uint2 x_bounds_u = convert_uint2(clamp((int2)(floor(x_bounds.x), ceil(x_bounds.y)),
												 0, screen_size.x - 1)); // valid pixel pos: [0, screen_size.x-1]
	const uint2 y_bounds_u = convert_uint2(clamp((int2)(floor(y_bounds.x), ceil(y_bounds.y)),
												 0, screen_size.y - 1));
	
	/*printf("[%d] (%u %u) (%u %u)\n",
		   triangle_id,
		   x_bounds_u.x, x_bounds_u.y,
		   y_bounds_u.x, y_bounds_u.y);
	
	printf("[%d] passing: %u // %u // %u %u %u\n",
		   triangle_id, passing_direct, clipped_count,
		   passing_indices[0], passing_indices[1], passing_indices[2]);*/
	
	/*float area = 0.0f;
	if(passing_indices[0] > 0 && passing_indices[1] > 0 && passing_indices[2] > 0) {
		// TODO: actually: if any edge must be clipped -> triangle area has to be big enough
		// otherwise, there wouldn't be any clipping necessary
		// -> only compute area if all 3 edges/vertices pass directly
		// however: all edges must be valid in the first place! (check precision?)
		const float2 e0 = clipping_coords[1] - clipping_coords[0];
		const float2 e1 = clipping_coords[2] - clipping_coords[0];
		area = 0.5f * (e0.x * e1.y - e0.y * e1.x);
		//printf("[%d] %f %f -> %f %f\n", triangle_id, e0.x, e0.y, e1.x, e1.y);
		if(area < 0.5f) { // half sample size (TODO: -> check if between sample points)
			//discard();
		}
		//printf("[%d] area: %f\n", triangle_id, area);
	}*/
	
	// TODO: determine triangle backside
	
	// TODO: already read depth from framebuffer in here -> cull if greater depth
	/*if((x_bounds_u.y >= (screen_size.x - 1)) ||
	   (y_bounds_u.y >= (screen_size.y - 1))) {
	//if((x_bounds_u.y >= (screen_size.x - 1) &&
	//	x_bounds_u.x < x_bounds_u.y) ||
	//   (y_bounds_u.y >= (screen_size.y - 1) &&
	//	y_bounds_u.x < y_bounds_u.y)) {
		printf("[%d] bounds: %u %u /// %u %u\n",
			   triangle_id,
			   x_bounds_u.x, x_bounds_u.y,
			   y_bounds_u.x, y_bounds_u.y);
	}*/
	// TODO: find binning error (possibly memory fault)
	
	// insert triangle id intro appropriate queues/bins
	const uint2 x_bins = x_bounds_u / tile_size.x;
	const uint2 y_bins = y_bounds_u / tile_size.y;
	const unsigned int queue_size = triangle_count; // TODO: only true for now
	
	for(unsigned int y_bin = y_bins.x; y_bin <= y_bins.y; y_bin++) {
		for(unsigned int x_bin = x_bins.x; x_bin <= x_bins.y; x_bin++) {
			const unsigned int bin_index = y_bin * bin_count.x + x_bin;
			const unsigned int queue_index = atomic_inc(&queue_sizes_buffer[bin_index]);
			const unsigned int queue_offset = (queue_size * bin_index) + queue_index;
			triangle_queues_buffer[queue_offset] = triangle_id;
		}
	}
}
