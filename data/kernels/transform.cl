
#include "oclr_global.h"
#include "oclr_math.h"
#include "oclr_matrix.h"

typedef struct __attribute__((packed)) {
	// note: don't use float3 (-> will be aligned as float4, even with packed attribute)
	float4 vertex;
	float4 normal;
	float4 binormal;
	float4 tangent;
	//float2 tex_coord;
} vertex_data;

typedef struct __attribute__((packed)) {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
} constant_data;

typedef struct __attribute__((packed)) {
	float4 VV0;
	float4 VV1;
	float4 VV2;
	float4 W;
} transformed_data;

//
kernel void transform(global const vertex_data* vertex_buffer,
					  global const unsigned int* index_buffer,
					  global transformed_data* transformed_buffer,
					  constant constant_data* cdata,
					  const unsigned int triangle_count) {
	const size_t triangle_id = get_global_id(0);
	// global work size is greater than the actual triangle count
	// -> check for triangle_count instead of get_global_size(0)
	if(triangle_id >= triangle_count) return;
	
	const unsigned int indices[3] = {
		index_buffer[triangle_id*3],
		index_buffer[triangle_id*3 + 1],
		index_buffer[triangle_id*3 + 2]
	};
	
	//
	const float3 D0 = cdata->camera_origin.xyz;
	const float3 DX = cdata->camera_x_vec.xyz;
	const float3 DY = cdata->camera_y_vec.xyz;
	const float3 VE = cdata->camera_position.xyz;
	float3 vertices[3] = {
		vertex_buffer[indices[0]].vertex.xyz - VE,
		vertex_buffer[indices[1]].vertex.xyz - VE,
		vertex_buffer[indices[2]].vertex.xyz - VE
	};
	
	/*print_float3(D0);
	print_float3(DX);
	print_float3(DY);
	print_float3(VE);*/
	
	// since VE0 can be interpreted as (0, 0, 0, 1) after it has been substracted from the vertices,
	// the original algorithm (requiring the computation of 4x4 matrix determinants) can be simplified:
	float4 VV0 = (float4)(det3(DX, vertices[1], vertices[2]),
						  det3(DY, vertices[1], vertices[2]),
						  det3(D0, vertices[1], vertices[2]),
						  det3(vertices[0], vertices[1], vertices[2]));
	float4 VV1 = (float4)(det3(vertices[0], DX, vertices[2]),
						  det3(vertices[0], DY, vertices[2]),
						  det3(vertices[0], D0, vertices[2]),
						  0.0f);
	float4 VV2 = (float4)(det3(vertices[0], vertices[1], DX),
						  det3(vertices[0], vertices[1], DY),
						  det3(vertices[0], vertices[1], D0),
						  0.0f);
	
	//printf("sizes: %u %u %u\n", sizeof(vertex_data), sizeof(constant_data), sizeof(transformed_data));
	/*printf("%u: (%f %f %f %f) (%f %f %f %f) (%f %f %f %f) (%f %f %f %f)\n",
		   triangle_id,
		   VV0.x, VV0.y, VV0.z, VV0.w,
		   VV1.x, VV1.y, VV1.z, VV1.w,
		   VV2.x, VV2.y, VV2.z, VV2.w,
		   W.x, W.y, W.z, W.w);*/
	/*printf("%u: (%f %f %f %f) (%f %f %f %f) (%f %f %f %f)\n",
		   triangle_id,
		   VV0.x, VV0.y, VV0.z, VV0.w,
		   VV1.x, VV1.y, VV1.z, VV1.w,
		   VV2.x, VV2.y, VV2.z, VV2.w);*/
	
	transformed_buffer[triangle_id].VV0 = VV0;
	transformed_buffer[triangle_id].VV1 = VV1;
	transformed_buffer[triangle_id].VV2 = VV2;
	transformed_buffer[triangle_id].W = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
}
