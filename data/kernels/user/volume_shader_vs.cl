
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
	float2 tex_coord;
} input_attributes;

oclraster_out simple_output {
	float2 tex_coord;
} output_attributes;

float4 transform_main() {
	output_attributes->tex_coord = input_attributes->tex_coord;
	return input_attributes->vertex;
}
