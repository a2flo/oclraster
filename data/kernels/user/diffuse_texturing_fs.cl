
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_out simple_output {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms rasterize_uniforms {
	float4 camera_position;
	float4 light_position; // .w = light radius ^ 2
	float4 light_color;
} rp_uniforms;

oclraster_images {
	read_only image2d diffuse_texture;
};
oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

void rasterize_main() {
	const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
	const float4 color = image_read(diffuse_texture, sampler, output_attributes->tex_coord);
	framebuffer->color = color;
}
