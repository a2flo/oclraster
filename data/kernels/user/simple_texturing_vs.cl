
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
	float4 normal;
	float4 binormal;
	float4 tangent;
	float2 tex_coord;
} input_attributes;

oclraster_out simple_output {
	float4 vertex;
	float4 view_vec;
	float4 normal;
	float4 binormal;
	float4 tangent;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms transform_uniforms {
	mat4 rotation_scale_matrix;
	mat4 modelview_matrix;
} tp_uniforms;

void transform_main() {
	const float4 mv_vertex = mat4_mul_vec4(tp_uniforms->modelview_matrix,
										   input_attributes->vertex);
	output_attributes->vertex = mv_vertex;
	
	//
	mat3 nbt_mat;
	nbt_mat.m[0] = normalize(mat4_mul_vec3(tp_uniforms->rotation_scale_matrix,
										   input_attributes->tangent.xyz));
	nbt_mat.m[1] = normalize(mat4_mul_vec3(tp_uniforms->rotation_scale_matrix,
										   input_attributes->binormal.xyz));
	nbt_mat.m[2] = normalize(mat4_mul_vec3(tp_uniforms->rotation_scale_matrix,
										   input_attributes->normal.xyz));
	
	output_attributes->view_vec = (float4)(vec3_mul_mat3(*VE - mv_vertex.xyz, nbt_mat), 1.0f);
	output_attributes->tangent = (float4)(nbt_mat.m[0], 1.0f);
	output_attributes->binormal = (float4)(nbt_mat.m[1], 1.0f);
	output_attributes->normal = (float4)(nbt_mat.m[2], 1.0f);
	output_attributes->tex_coord = input_attributes->tex_coord;
	
	transform(mv_vertex);
}
