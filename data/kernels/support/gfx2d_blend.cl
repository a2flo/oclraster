
oclraster_out simple_output {
	float2 tex_coord;
} output_attributes;

#if defined(OCLRASTER_TRANSFORM_PROGRAM)
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
} input_attributes;

float4 transform_main() {
	output_attributes->tex_coord = clamp(input_attributes->vertex.xy, (float2)(0.0f), (float2)(1.0f));
	return input_attributes->vertex;
}

#elif defined(OCLRASTER_RASTERIZATION_PROGRAM)
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_images {
	read_only image2d src_buffer;
};

oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

void rasterization_main() {
	const sampler_t point_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
	const float4 src_color = image_read(src_buffer, point_sampler, (float2)(output_attributes->tex_coord.x, 1.0f - output_attributes->tex_coord.y));
	//framebuffer->color.xyz = src_color.xyz + framebuffer->color.xyz * (1.0f - src_color.w);
	framebuffer->color.xyz = src_color.xyz;
	framebuffer->color.w = 1.0f;
}

#endif
