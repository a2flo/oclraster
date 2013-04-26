
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
	// NOTE: hints are optional (will default to <UINT_8, RGBA>)
	read_only image2d<UINT_8, RGBA> diffuse_texture;
};
oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

bool rasterize_main() {
	//framebuffer->color = (float4)(fragment_coord.x, fragment_coord.y, framebuffer->depth, 1.0f);
#if 0
	int depth_exp = 0;
	const float depth_mantissa = frexp(framebuffer->depth, &depth_exp);
	framebuffer->color = (float4)(log((float)depth_exp),
								  depth_mantissa, 0.0f, 1.0f);
#elif 0
	//if(framebuffer->depth < 0.0f) return;
	framebuffer->color = (float4)(framebuffer->depth / 8.0f,
								  framebuffer->depth / 4.0f,
								  framebuffer->depth / 2.0f, 1.0f);
	if(framebuffer->depth < 0.0f) {
		framebuffer->color = (float4)(1.0f, 0.0f, 1.0f, 1.0f);
	}
#elif 1
	float4 color = image_read_float_nearest(diffuse_texture, output_attributes->tex_coord);
	framebuffer->color = mix(color, framebuffer->color, 0.75f);
#elif 0
	const float frag_add = 1.0f/16.0f;
	framebuffer->color.xyz += frag_add;
	framebuffer->color.w = 1.0f;
#endif
	return true;
}
