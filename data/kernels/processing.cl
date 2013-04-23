#include "oclr_global.h"
#include "oclr_math.h"
#include "oclr_primitive_assembly.h"

typedef struct __attribute__((packed, aligned(16))) {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
	float4 camera_forward;
	float4 frustum_normals[3];
	uint2 viewport;
} constant_data;

typedef struct __attribute__((packed, aligned(4))) {
	// VV0: 0 - 2
	// VV1: 3 - 5
	// VV2: 6 - 8
	// depth: 9
	float data[10];
} transformed_data;

typedef struct __attribute__((packed, aligned(16))) {
	float4 bounds; // (.x = INFINITY if culled)
} triangle_bounds;

//
#define MIN_FRAGMENT_SIZE (1.0f / 256.0f)
#define discard() { tb_ptr->bounds.x = INFINITY; return; }
kernel void oclraster_processing(global const unsigned int* index_buffer,
								 global const float4* transformed_vertex_buffer,
								 global transformed_data* transformed_buffer,
								 global triangle_bounds* triangle_bounds_buffer,
								 constant constant_data* cdata,
								 const unsigned int primitive_type,
								 const unsigned int primitive_count,
								 const uint4 scissor_rectangle) {
	const unsigned int triangle_id = get_global_id(0);
	// global work size is greater than the actual primitive count
	// -> check for primitive_count instead of get_global_size(0)
	if(triangle_id >= primitive_count) return;
	global transformed_data* tf_ptr = &transformed_buffer[triangle_id];
	global triangle_bounds* tb_ptr = &triangle_bounds_buffer[triangle_id];
	global float* tf_data_ptr = tf_ptr->data;
	
	//
	MAKE_PRIMITIVE_INDICES(indices);
	
	//
#if defined(OCLRASTER_PROJECTION_PERSPECTIVE)
	const float3 D0 = cdata->camera_origin.xyz;
	const float3 DX = cdata->camera_x_vec.xyz;
	const float3 DY = cdata->camera_y_vec.xyz;
	const float3 forward = cdata->camera_forward.xyz;
#elif defined(OCLRASTER_PROJECTION_ORTHOGRAPHIC)
	const float2 D0 = cdata->camera_origin.xy;
	const float DX = cdata->camera_x_vec.x;
	const float DY = cdata->camera_y_vec.y;
#endif
	
	// read user transformed vertices
	const float3 vertices[3] = {
		transformed_vertex_buffer[indices[0]].xyz,
		transformed_vertex_buffer[indices[1]].xyz,
		transformed_vertex_buffer[indices[2]].xyz
	};
	
	// check if any vertex has been discarded (-> discard the triangle)
	for(unsigned int i = 0; i < 3; i++) {
		if(vertices[i].x == INFINITY) discard();
	}
	
#if defined(OCLRASTER_PROJECTION_PERSPECTIVE)
	// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
	const float triangle_near_clipping[3] = {
		dot(vertices[0], forward),
		dot(vertices[1], forward),
		dot(vertices[2], forward)
	};
	
	// if xyz < 0, don't add the triangle in the first place
	if(triangle_near_clipping[0] < 0.0f &&
	   triangle_near_clipping[1] < 0.0f &&
	   triangle_near_clipping[2] < 0.0f) {
		// all vertices are behind the camera
		discard();
	}
	
	// frustum culling using the "p/n-test"
	// also thx to ryg for figuring out this optimized version of the p/n-test
	const float3 aabb_min = fmin(fmin(vertices[0], vertices[1]), vertices[2]);
	const float3 aabb_max = fmax(fmax(vertices[0], vertices[1]), vertices[2]);
	const float3 aabb_center = (aabb_max + aabb_min); // note: actual scale doesn't matter (-> no *0.5f)
	const float3 aabb_extent = (aabb_max - aabb_min); // extent must have the same scale though
	float4 fc_dot = (float4)(0.0f, 0.0f, 0.0f, 0.0f);
	for(unsigned int i = 0; i < 3; i++) {
		const float4 plane_normal = cdata->frustum_normals[i];
		const uint4 plane_sign = *(const uint4*)&plane_normal & (uint4)(0x80000000);
		const uint4 flipped_extent = (uint4)(((const uint*)&aabb_extent)[i]) ^ plane_sign;
		const float4 dot_param = (float4)(((const float*)&aabb_center)[i]) + *(const float4*)&flipped_extent;
		fc_dot += dot_param * plane_normal;
	}
	// if any dot product is less than 0 (aabb is completely outside any plane) -> cull
	if(any(signbit(fc_dot))) {
		discard();
	}
#endif
	
#if defined(OCLRASTER_PROJECTION_PERSPECTIVE)
	// this uses the non-homogeneous version of the 3D rasterization algorithm (-> computing 4x4 matrix determinats is not required)
	// also: substracting VE in here is not necessary, since it has already been done in the transform stage
	const float3 c01 = cross(vertices[0] - vertices[1], vertices[1]);
	const float3 c20 = cross(vertices[2] - vertices[0], vertices[0]);
	const float3 c12 = cross(vertices[1] - vertices[2], vertices[2]);
	const float o01 = dot(D0, c01);
	const float o20 = dot(D0, c20);
	const float o12 = dot(D0, c12);
	const float x01 = dot(DX, c01);
	const float x20 = dot(DX, c20);
	const float x12 = dot(DX, c12);
	const float y01 = dot(DY, c01);
	const float y20 = dot(DY, c20);
	const float y12 = dot(DY, c12);
	
	// TODO: compute triangle area through dx/dy -> vertices diff?
	
	//
	const float VV[3][3] = {
		{ x12, y12, o12 },
		{ x20, y20, o20 },
		{ x01, y01, o01 }
	};
	const float VV_depth = dot(vertices[0], c12);
#elif defined(OCLRASTER_PROJECTION_ORTHOGRAPHIC)
	// TODO: D0 offset
	
	const float x0 = DX * (vertices[2].y - vertices[1].y);
	const float x1 = DX * (vertices[0].y - vertices[2].y);
	const float x2 = DX * (vertices[1].y - vertices[0].y);
	
	const float y0 = DY * (vertices[1].x - vertices[2].x);
	const float y1 = DY * (vertices[2].x - vertices[0].x);
	const float y2 = DY * (vertices[0].x - vertices[1].x);
	
	const float o0 = -mad(vertices[2].x, x0, (vertices[2].y * y0));
	const float o1 = -mad(vertices[0].x, x1, (vertices[0].y * y1));
	const float o2 = -mad(vertices[1].x, x2, (vertices[1].y * y2));
	
	const float VV[3][3] = {
		{ x0, y0, o0 },
		{ x1, y1, o1 },
		{ x2, y2, o2 }
	};
	const float VV_depth = dot(vertices[0], cross(vertices[1], vertices[2]));
#endif
	
	/*for(uint i = 0; i < 3; i++) {
		printf("[%d] VV%d: %.10f %.10f %.10f\n", triangle_id, i, VV[i][0], VV[i][1], VV[i][2]);
	}*/
	
#if defined(OCLRASTER_PROJECTION_PERSPECTIVE)
	// clipping / additional culling
	// NOTE/disclaimer: I wanted to make this nice and put it into a clipping function,
	// but apparently nobody is capable of writing a c compiler without massive bugs
	// also: if your linker is crashing, at least output a symbol name, so that I can
	// write a workaround (I'm looking at you, AMD!)
	const float fscreen_size[2] = { convert_float(cdata->viewport.x), convert_float(cdata->viewport.y) };
	float2 x_bounds = (float2)(fscreen_size[0], 0.0f);
	float2 y_bounds = (float2)(fscreen_size[1], 0.0f);
	{
		// imprecision culling (this will really mess up the later pipeline if it isn't culled here)
		// also note that all values must be below the epsilon, otherwise these might be valid for clipping
#define VV_EPSILON 0.00001f
#define VV_EPSILON_9 0.00009f
		//if(fabs(VV[0].x) <= VV_EPSILON && fabs(VV[0].y) <= VV_EPSILON && fabs(VV[0].z) <= VV_EPSILON &&
		//   fabs(VV[1].x) <= VV_EPSILON && fabs(VV[1].y) <= VV_EPSILON && fabs(VV[1].z) <= VV_EPSILON &&
		//   fabs(VV[2].x) <= VV_EPSILON && fabs(VV[2].y) <= VV_EPSILON && fabs(VV[2].z) <= VV_EPSILON) {
		// TODO: dot product might also work (even lower epsilon though)
		// TODO: epsilon too high?
		/*if(((fabs(VV[0][0]) + fabs(VV[0][1])) +
		 (fabs(VV[0][2]) + fabs(VV[1][0])) +
		 (fabs(VV[1][1]) + fabs(VV[1][2])) +
		 (fabs(VV[2][0]) + fabs(VV[2][1]) + fabs(VV[2][2]))) <= VV_EPSILON_9) {
		 //printf("imprecision culled (tp1): %d\n", triangle_id);
		 return;
		 }*/
		
		// triangle area and backface culling:
		// note: in this stage, this requires the triangle to be completely visible (no clipping)
		
#define viewport_test(coord, axis) ((coord < 0.0f || coord >= fscreen_size[axis]) ? -1.0f : coord)
		float coord_xs[9];
		float coord_ys[9];
		//float2 clipping_coords[3] = { (float2)(0.0f, 0.0f), (float2)(0.0f, 0.0f), (float2)(0.0f, 0.0f) };
		//unsigned int passing_indices[3] = { 0, 0, 0 };
		unsigned int valid_coords = 0;
		//unsigned int clipped_count = 0;
		for(unsigned int i = 0u; i < 3u; i++) {
			// { 1, 2 }, { 0, 2 }, { 0, 1 }
			const unsigned int i0 = (i == 0u ? 1u : 0u);
			const unsigned int i1 = (i == 2u ? 1u : 2u);
			
			const float d = 1.0f / (VV[i0][0] * VV[i1][1] - VV[i0][1] * VV[i1][0]);
			coord_xs[i] = (triangle_near_clipping[i] < 0.0f ? -1.0f :
						   (VV[i0][1] * VV[i1][2] - VV[i0][2] * VV[i1][1]) * d);
			coord_ys[i] = (triangle_near_clipping[i] < 0.0f ? -1.0f :
						   (VV[i0][2] * VV[i1][0] - VV[i0][0] * VV[i1][2]) * d);
			coord_xs[i] = viewport_test(coord_xs[i], 0);
			coord_ys[i] = viewport_test(coord_ys[i], 1);
			
			if(coord_xs[i] >= 0.0f && coord_ys[i] >= 0.0f) {
				valid_coords++;
				
				x_bounds.x = min(x_bounds.x, coord_xs[i]);
				x_bounds.y = max(x_bounds.y, coord_xs[i]);
				y_bounds.x = min(y_bounds.x, coord_ys[i]);
				y_bounds.y = max(y_bounds.y, coord_ys[i]);
				//passing_indices[i]++;
				//clipping_coords[i] = (float2)(coord_xs[i], coord_ys[i]);
			}
			
			//
			coord_xs[i+3] = -VV[i][2] / VV[i][0];
			coord_ys[i+3] = -VV[i][2] / VV[i][1];
			coord_xs[i+3] = viewport_test(coord_xs[i+3], 0);
			coord_ys[i+3] = viewport_test(coord_ys[i+3], 1);
			
			coord_xs[i+6] = -(VV[i][2] + VV[i][1] * fscreen_size[1]) / VV[i][0];
			coord_ys[i+6] = -(VV[i][2] + VV[i][0] * fscreen_size[0]) / VV[i][1];
			coord_xs[i+6] = viewport_test(coord_xs[i+6], 0);
			coord_ys[i+6] = viewport_test(coord_ys[i+6], 1);
		}
		
		if(valid_coords == 3) {
			// compute triangle area if all vertices pass
			const float2 e0 = (float2)(coord_xs[1] - coord_xs[0],
									   coord_ys[1] - coord_ys[0]);
			const float2 e1 = (float2)(coord_xs[2] - coord_xs[0],
									   coord_ys[2] - coord_ys[0]);
			const float area = -0.5f * (e0.x * e1.y - e0.y * e1.x);
			// half sample size (TODO: -> check if between sample points; <=1/2^8 sample size seems to be a good threshold?)
			if(area < MIN_FRAGMENT_SIZE) {
				//printf("triangle area culled: %d\n", triangle_id);
				discard(); // cull
			}
		}
		
		// second imprecision culling:
		// if all direct clip/vertex screen space coordinates are invalid or 0 -> cull
		if((coord_xs[0] == 0.0f || coord_xs[0] == -1.0f) &&
		   (coord_xs[1] == 0.0f || coord_xs[1] == -1.0f) &&
		   (coord_xs[2] == 0.0f || coord_xs[2] == -1.0f) &&
		   (coord_ys[0] == 0.0f || coord_ys[0] == -1.0f) &&
		   (coord_ys[1] == 0.0f || coord_ys[1] == -1.0f) &&
		   (coord_ys[2] == 0.0f || coord_ys[2] == -1.0f)) {
			//printf("imprecision culled (tp2): %d\n", triangle_id);
			discard();
		}
		
		// ---bin---
		// check remaining clip coordinates
		for(unsigned int i = 0; i < 3; i++) {
			const float cx = coord_xs[i + 3];
			const float cy = coord_ys[i + 3];
			const float cmx = coord_xs[i + 6];
			const float cmy = coord_ys[i + 6];
			
			const unsigned int edge_0 = (i + 1) % 3;
			const unsigned int edge_1 = (i + 2) % 3;
			
			if(cx >= 0.0f) {
				float val1 = cx * VV[edge_0][0] + VV[edge_0][2];
				float val2 = cx * VV[edge_1][0] + VV[edge_1][2];
				if(val1 < 0.0f && val2 < 0.0f) {
					x_bounds.x = min(x_bounds.x, cx);
					x_bounds.y = max(x_bounds.y, cx);
					y_bounds.x = 0.0f;
					//passing_indices[i]++;
					//clipped_count++;
					//clipping_coords[i] = (float2)(cx, 0.0f);
				}
			}
			if(cy >= 0.0f) {
				float val1 = cy * VV[edge_0][1] + VV[edge_0][2];
				float val2 = cy * VV[edge_1][1] + VV[edge_1][2];
				if(val1 < 0.0f && val2 < 0.0f) {
					y_bounds.x = min(y_bounds.x, cy);
					y_bounds.y = max(y_bounds.y, cy);
					x_bounds.x = 0.0f;
					//passing_indices[i]++;
					//clipped_count++;
					//clipping_coords[i] = (float2)(0.0f, cy);
				}
			}
			if(cmx >= 0.0f) {
				float val1 = cmx * VV[edge_0][0] + fscreen_size[1] * VV[edge_0][1] + VV[edge_0][2];
				float val2 = cmx * VV[edge_1][0] + fscreen_size[1] * VV[edge_1][1] + VV[edge_1][2];
				if(val1 < 0.0f && val2 < 0.0f) {
					x_bounds.x = min(x_bounds.x, cmx);
					x_bounds.y = max(x_bounds.y, cmx);
					y_bounds.y = fscreen_size[1];
					//passing_indices[i]++;
					//clipped_count++;
					//clipping_coords[i] = (float2)(cmx, 0.0f);
				}
			}
			if(cmy >= 0.0f) {
				float val1 = fscreen_size[0] * VV[edge_0][0] + cmy * VV[edge_0][1] + VV[edge_0][2];
				float val2 = fscreen_size[0] * VV[edge_1][0] + cmy * VV[edge_1][1] + VV[edge_1][2];
				if(val1 < 0.0f && val2 < 0.0f) {
					y_bounds.x = min(y_bounds.x, cmy);
					y_bounds.y = max(y_bounds.y, cmy);
					x_bounds.y = fscreen_size[0];
					//passing_indices[i]++;
					//clipped_count++;
					//clipping_coords[i] = (float2)(0.0f, cmy);
				}
			}
		}
	}
#elif defined(OCLRASTER_PROJECTION_ORTHOGRAPHIC)
	const float2 aabb_min = fmin(fmin(vertices[0].xy, vertices[1].xy), vertices[2].xy);
	const float2 aabb_max = fmax(fmax(vertices[0].xy, vertices[1].xy), vertices[2].xy);
	float2 x_bounds = (float2)(aabb_min.x, aabb_max.x);
	float2 y_bounds = (float2)(aabb_min.y, aabb_max.y);
	if(fabs(aabb_min.x - aabb_max.x) < MIN_FRAGMENT_SIZE ||
	   fabs(aabb_min.y - aabb_max.y) < MIN_FRAGMENT_SIZE) {
		discard();
	}
#endif
	const float4 bounds = (float4)(floor(x_bounds.x), ceil(x_bounds.y),
								   floor(y_bounds.x), ceil(y_bounds.y));
	
#if defined(OCLRASTER_PROJECTION_ORTHOGRAPHIC)
	// scissor test
	const uint4 ubounds = convert_uint4(bounds);
	if(scissor_rectangle.x > ubounds.y || ubounds.x > scissor_rectangle.z ||
	   scissor_rectangle.y > ubounds.w || ubounds.z > scissor_rectangle.w) {
		discard();
	}
#endif
	
	// output:
	for(unsigned int i = 0u; i < 3u; i++) {
		*tf_data_ptr++ = VV[i][0];
		*tf_data_ptr++ = VV[i][1];
		*tf_data_ptr++ = VV[i][2];
	}
	//printf("[%d] bounds: %f %f -> %f %f\n", triangle_id, x_bounds.x, y_bounds.x, x_bounds.y, y_bounds.y);
	*tf_data_ptr++ = VV_depth;
	
	// TODO: rounding should depend on sampling mode
	tb_ptr->bounds = bounds;
}
