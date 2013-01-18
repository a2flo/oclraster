
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
	
	const float4 VV0 = transformed_buffer[triangle_id].VV0;
	const float4 VV1 = transformed_buffer[triangle_id].VV1;
	const float4 VV2 = transformed_buffer[triangle_id].VV2;
	const float3 VV[3] = { VV0.xyz, VV1.xyz, VV2.xyz };
	
	// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
	const float4 vertex_cam_relation = transformed_buffer[triangle_id].W;
	if(vertex_cam_relation.x < 0.0f &&
	   vertex_cam_relation.y < 0.0f &&
	   vertex_cam_relation.z < 0.0f) {
		// all vertices are behind the camera
		return;
	}
	
	// compute x/y bounds
	const float clip_x0 = -VV0.z / VV0.x;
	const float clip_x1 = -VV1.z / VV1.x;
	const float clip_x2 = -VV2.z / VV2.x;
	const float clip_y0 = -VV0.z / VV0.y;
	const float clip_y1 = -VV1.z / VV1.y;
	const float clip_y2 = -VV2.z / VV2.y;
	
	const float2 fscreen_size = convert_float2(screen_size);
	const float clip_v_x0 = -(VV0.z + VV0.y * fscreen_size.y) / VV0.x;
	const float clip_v_x1 = -(VV1.z + VV1.y * fscreen_size.y) / VV1.x;
	const float clip_v_x2 = -(VV2.z + VV2.y * fscreen_size.y) / VV2.x;
	const float clip_v_y0 = -(VV0.z + VV0.x * fscreen_size.x) / VV0.y;
	const float clip_v_y1 = -(VV1.z + VV1.x * fscreen_size.x) / VV1.y;
	const float clip_v_y2 = -(VV2.z + VV2.x * fscreen_size.x) / VV2.y;
	
	const float d0 = 1.0f / (VV1.x * VV2.y - VV1.y * VV2.x);
	const float d1 = 1.0f / (VV0.x * VV2.y - VV0.y * VV2.x);
	const float d2 = 1.0f / (VV0.x * VV1.y - VV0.y * VV1.x);
	const float x0 = (vertex_cam_relation.x < 0.0f ?
					  -1.0f : (VV1.y * VV2.z - VV1.z * VV2.y) * d0);
	const float y0 = (vertex_cam_relation.x < 0.0f ?
					  -1.0f : (VV1.z * VV2.x - VV1.x * VV2.z) * d0);
	const float x1 = (vertex_cam_relation.y < 0.0f ?
					  -1.0f : (VV0.y * VV2.z - VV0.z * VV2.y) * d1);
	const float y1 = (vertex_cam_relation.y < 0.0f ?
					  -1.0f : (VV0.z * VV2.x - VV0.x * VV2.z) * d1);
	const float x2 = (vertex_cam_relation.z < 0.0f ?
					  -1.0f : (VV0.y * VV1.z - VV0.z * VV1.y) * d2);
	const float y2 = (vertex_cam_relation.z < 0.0f ?
					  -1.0f : (VV0.z * VV1.x - VV0.x * VV1.z) * d2);
	
	float xs[9] = {
		x0, x1, x2,
		clip_x0, clip_x1, clip_x2,
		clip_v_x0, clip_v_x1, clip_v_x2
	};
	
	float ys[9] = {
		y0, y1, y2,
		clip_y0, clip_y1, clip_y2,
		clip_v_y0, clip_v_y1, clip_v_y2
	};
	
	for(unsigned int i = 0; i < 9; i++) {
		if(xs[i] < 0.0f || xs[i] >= fscreen_size.x) {
			xs[i] = -1.0f;
		}
		if(ys[i] < 0.0f || ys[i] >= fscreen_size.y) {
			ys[i] = -1.0f;
		}
	}
	
	////
	// valid clip and vertex positions: 0 <= x < screen_size.x && 0 <= y < screen_size.y
	float2 x_bounds = (float2)(fscreen_size.x, 0.0f);
	float2 y_bounds = (float2)(fscreen_size.y, 0.0f);
	
	//
	for(unsigned int i = 0; i < 3; i++) {
		const float xi = xs[i];
		const float yi = ys[i];
		
		if(xi >= 0.0f && yi >= 0.0f) {
			x_bounds.x = min(x_bounds.x, xi);
			x_bounds.y = max(x_bounds.y, xi);
			y_bounds.x = min(y_bounds.x, yi);
			y_bounds.y = max(y_bounds.y, yi);
		}
	}
	
	for(unsigned int i = 0; i < 3; i++) {
		const float cx = xs[i + 3];
		const float cy = ys[i + 3];
		const float cmx = xs[i + 6];
		const float cmy = ys[i + 6];
		
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
