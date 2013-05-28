
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
	float4 normal;
	float4 binormal; // unused
	float4 tangent; // unused
	float2 tex_coord;
} input_attributes;

oclraster_out simple_output {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms transform_uniforms {
	mat4 modelview_matrix;
} tp_uniforms;

float4 transform_main() {
	output_attributes->normal = input_attributes->normal;
	output_attributes->tex_coord = input_attributes->tex_coord;
	
	float4 mv_vertex = mat4_mul_vec4(tp_uniforms->modelview_matrix,
									 input_attributes->vertex);
	output_attributes->vertex = mv_vertex;
	return mv_vertex;
}
