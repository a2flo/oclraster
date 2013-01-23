
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
	
	const float3 VV[3] = {
		transformed_buffer[triangle_id].VV0.xyz,
		transformed_buffer[triangle_id].VV1.xyz,
		transformed_buffer[triangle_id].VV2.xyz
	};
	
	// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
	const float4 vertex_cam_relation = transformed_buffer[triangle_id].W;
	if(vertex_cam_relation.x < 0.0f &&
	   vertex_cam_relation.y < 0.0f &&
	   vertex_cam_relation.z < 0.0f) {
		// all vertices are behind the camera
		return;
	}
	
	// compute x/y bounds
	// valid clip and vertex positions: 0 <= x < screen_size.x && 0 <= y < screen_size.y
	const float2 fscreen_size = convert_float2(screen_size);
	float2 x_bounds = (float2)(fscreen_size.x, 0.0f);
	float2 y_bounds = (float2)(fscreen_size.y, 0.0f);
	
#define viewport_test(coord, axis) ((coord < 0.0f || coord >= fscreen_size[axis]) ? -1.0f : coord)
	
	float clipxs[9];
	float clipys[9];
	for(unsigned int i = 0u; i < 3u; i++) {
		// { 1, 2 }, { 0, 2 }, { 0, 1 }
		const unsigned int i0 = (i == 0u ? 1u : 0u);
		const unsigned int i1 = (i == 2u ? 1u : 2u);
		
		const float d = 1.0f / (VV[i0].x * VV[i1].y - VV[i0].y * VV[i1].x);
		clipxs[i] = (vertex_cam_relation[i] < 0.0f ? -1.0f :
					 (VV[i0].y * VV[i1].z - VV[i0].z * VV[i1].y) * d);
		clipys[i] = (vertex_cam_relation[i] < 0.0f ? -1.0f :
					 (VV[i0].z * VV[i1].x - VV[i0].x * VV[i1].z) * d);
		clipxs[i] = viewport_test(clipxs[i], 0);
		clipys[i] = viewport_test(clipys[i], 1);
		
		if(clipxs[i] >= 0.0f && clipys[i] >= 0.0f) {
			x_bounds.x = min(x_bounds.x, clipxs[i]);
			x_bounds.y = max(x_bounds.y, clipxs[i]);
			y_bounds.x = min(y_bounds.x, clipys[i]);
			y_bounds.y = max(y_bounds.y, clipys[i]);
		}
		
		//
		clipxs[i+3] = -VV[i].z / VV[i].x;
		clipys[i+3] = -VV[i].z / VV[i].y;
		clipxs[i+3] = viewport_test(clipxs[i+3], 0);
		clipys[i+3] = viewport_test(clipys[i+3], 1);
		
		clipxs[i+6] = -(VV[i].z + VV[i].y * fscreen_size.y) / VV[i].x;
		clipys[i+6] = -(VV[i].z + VV[i].x * fscreen_size.x) / VV[i].y;
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
				x_bounds.x = min(x_bounds.x, cx);
				x_bounds.y = max(x_bounds.y, cx);
				y_bounds.x = 0.0f;
			}
		}
		if(cy >= 0.0f) {
			float val1 = cy * VV[edge_0].y + VV[edge_0].z;
			float val2 = cy * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				y_bounds.x = min(y_bounds.x, cy);
				y_bounds.y = max(y_bounds.y, cy);
				x_bounds.x = 0.0f;
			}
		}
		if(cmx >= 0.0f) {
			float val1 = cmx * VV[edge_0].x + fscreen_size.y * VV[edge_0].y + VV[edge_0].z;
			float val2 = cmx * VV[edge_1].x + fscreen_size.y * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				x_bounds.x = min(x_bounds.x, cmx);
				x_bounds.y = max(x_bounds.y, cmx);
				y_bounds.y = fscreen_size.y;
			}
		}
		if(cmy >= 0.0f) {
			float val1 = fscreen_size.x * VV[edge_0].x + cmy * VV[edge_0].y + VV[edge_0].z;
			float val2 = fscreen_size.x * VV[edge_1].x + cmy * VV[edge_1].y + VV[edge_1].z;
			if(val1 < 0.0f && val2 < 0.0f) {
				y_bounds.x = min(y_bounds.x, cmy);
				y_bounds.y = max(y_bounds.y, cmy);
				x_bounds.y = fscreen_size.x;
			}
		}
	}
	
	const uint2 x_bounds_u = convert_uint2(clamp((int2)(floor(x_bounds.x), ceil(x_bounds.y)),
												 0, screen_size.x - 1)); // valid pixel pos: [0, screen_size.x-1]
	const uint2 y_bounds_u = convert_uint2(clamp((int2)(floor(y_bounds.x), ceil(y_bounds.y)),
												 0, screen_size.y - 1));
	
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
