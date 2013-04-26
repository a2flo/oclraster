
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_out simple_output {
	float2 tex_coord;
} output_attributes;

oclraster_images {
	read_only image2d texture;
};
oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

bool rasterize_main() {
	const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
	const float4 color = image_read(texture, sampler, output_attributes->tex_coord);
	framebuffer->color.xyz = color.xyz;
	return true;
}
