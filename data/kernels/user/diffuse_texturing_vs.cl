
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
	float2 tex_coord;
} output_attributes;

oclraster_uniforms transform_uniforms {
	mat4 rotation_matrix;
	mat4 modelview_matrix;
} tp_uniforms;

float4 transform_main() {
	output_attributes->tex_coord = input_attributes->tex_coord;
	return mat4_mul_vec4(tp_uniforms->modelview_matrix,
						 input_attributes->vertex);
}
