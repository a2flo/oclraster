
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
	float4 normal;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms transform_uniforms {
	mat4 rotation_matrix;
	mat4 modelview_matrix;
} tp_uniforms;

void transform_main() {
	const float4 mv_vertex = mat4_mul_vec4(tp_uniforms->modelview_matrix,
										   input_attributes->vertex);
	output_attributes->vertex = mv_vertex;
	output_attributes->normal = mat4_mul_vec4(tp_uniforms->rotation_matrix,
											  input_attributes->normal);
	output_attributes->tex_coord = input_attributes->tex_coord;
	
	transform(mv_vertex);
}
