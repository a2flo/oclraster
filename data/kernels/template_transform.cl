
#include "oclr_global.h"
#include "oclr_math.h"
#include "oclr_matrix.h"

typedef struct __attribute__((packed)) {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
	float4 camera_forward;
	uint2 viewport;
} constant_data;

typedef struct __attribute__((packed)) {
	float4 VV0;
	float4 VV1;
	float4 VV2;
	float4 W;
} transformed_data;

// transform rerouting
#define transform(vertex) _transform(vertex, VE, transformed_vertex)
OCLRASTER_FUNC void _transform(float4 vertex, const float3* VE, float3* transformed_vertex) {
	*transformed_vertex = vertex.xyz - *VE;
}

//###OCLRASTER_USER_CODE###
//////////////////////////////////////////////////////////////////
// transform program
oclraster_in {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} simple_input;

oclraster_out {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} simple_output;

oclraster_uniforms {
	mat4 modelview_matrix;
} transform_uniforms;

// void main() {
void user_main(const int index,
			   const simple_input* input_attributes,
			   global simple_output* output_attributes,
			   const transform_uniforms* simple_transform_uniforms,
			   const float3* VE,
			   float3* transformed_vertex) {
	output_attributes->normal = input_attributes->normal;
	output_attributes->tex_coord = input_attributes->tex_coord;
	
	float4 mv_vertex = mat4_mul_vec4(simple_transform_uniforms->modelview_matrix,
									 input_attributes->vertex);
	output_attributes->vertex = mv_vertex;
	transform(mv_vertex);
}

//
kernel void _transform_kernel(//###OCLRASTER_USER_STRUCTS###
							  global const simple_input* vertex_buffer,
							  global simple_output* vertex_output_buffer,
							  constant transform_uniforms* input_uniforms,
							  
							  global const unsigned int* index_buffer,
							  global transformed_data* transformed_buffer,
							  constant constant_data* cdata,
							  const unsigned int triangle_count) {
	const unsigned int triangle_id = get_global_id(0);
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
	const float3 forward = cdata->camera_forward.xyz;
	
	float3 vertices[3];
	const transform_uniforms uniforms = *input_uniforms;
	for(int i = 0; i < 3; i++) {
		//###OCLRASTER_USER_MAIN_CALL###
		simple_input input = vertex_buffer[indices[i]];
		global simple_output* output = &vertex_output_buffer[indices[i]];
		user_main(indices[i], &input, output, &uniforms, &VE, &vertices[i]);
	}
	
	// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
	const float4 triangle_cam_relation = (float4)(dot(vertices[0], forward),
												  dot(vertices[1], forward),
												  dot(vertices[2], forward),
												  0.0f);
	// TODO: if xyz < 0, don't add it in the first place
	
	// TODO: actual culling
	
	// since VE0 can be interpreted as (0, 0, 0, 1) after it has been substracted from the vertices,
	// the original algorithm (requiring the computation of 4x4 matrix determinants) can be simplified:
	const float3 c01 = cross(vertices[0], vertices[1]);
	const float3 c20 = cross(vertices[2], vertices[0]);
	const float3 c12 = cross(vertices[1], vertices[2]);
	const float o01 = dot(D0, c01);
	const float o20 = dot(D0, c20);
	const float o12 = dot(D0, c12);
	const float x01 = dot(DX, c01);
	const float x20 = dot(DX, c20);
	const float x12 = dot(DX, c12);
	const float y01 = dot(DY, c01);
	const float y20 = dot(DY, c20);
	const float y12 = dot(DY, c12);
	
	float4 VV0 = (float4)(x12, y12, o12, dot(vertices[0], c12));
	float4 VV1 = (float4)(x20, y20, o20, 0.0f);
	float4 VV2 = (float4)(x01, y01, o01, 0.0f);
	
	//
	transformed_buffer[triangle_id].VV0 = VV0;
	transformed_buffer[triangle_id].VV1 = VV1;
	transformed_buffer[triangle_id].VV2 = VV2;
	transformed_buffer[triangle_id].W = triangle_cam_relation; // TODO: don't call it .W any more
}
