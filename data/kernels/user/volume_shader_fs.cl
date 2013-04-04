
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_out simple_output {
	float2 tex_coord;
} output_attributes;

oclraster_images {
	read_only image2d volume_texture;
	read_only image2d tf_texture;
};
oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

void rasterize_main() {
	const sampler_t point_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
	const sampler_t linear_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
	
	const float val = image_read(volume_texture, linear_sampler, output_attributes->tex_coord).x;
	const float4 color = image_read(tf_texture, point_sampler, (float2)(val, 1.0f));
	
	const float blend_factor = color.w, blend_factor_n = 1.0f - blend_factor;
	framebuffer->color.xyz = clamp(color.xyz * blend_factor + framebuffer->color.xyz * blend_factor_n, 0.0f, 1.0f);
	framebuffer->color.w = 1.0f;
}
