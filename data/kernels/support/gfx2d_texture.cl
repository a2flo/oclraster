
#include "support/gfx2d.h"

#if defined(OCLRASTER_TRANSFORM_PROGRAM)
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
} input_attributes;

oclraster_uniforms transform_uniforms {
	mat4 modelview_matrix;
} tp_uniforms;

float4 transform_main() {
	return mat4_mul_vec4(tp_uniforms->modelview_matrix,
						 input_attributes->vertex);
}

#elif defined(OCLRASTER_RASTERIZATION_PROGRAM)
//////////////////////////////////////////////////////////////////
// rasterization program
#if (defined(TEXTURE_GRADIENT_HORIZONTAL) || \
	 defined(TEXTURE_GRADIENT_VERTICAL) || \
	 defined(TEXTURE_GRADIENT_DIAGONAL_LR) || \
	 defined(TEXTURE_GRADIENT_DIAGONAL_RL))
#define TEXTURE_GRADIENT
#endif

oclraster_uniforms rasterize_uniforms {
#if defined(TEXTURE_MADD_COLOR) || defined(TEXTURE_GRADIENT)
	float4 mul_color;
	float4 add_color;
#endif
	
#if defined(TEXTURE_GRADIENT)
	float4 gradient_mul_interpolator;
	float4 gradient_add_interpolator;
	
	float4 gradients[4]; // -> max gradient colors: 4
	float4 stops;
#endif
	
	float4 extent;
	float4 orientation;
	float draw_depth; // TODO: check if this is correct/usable!
	
#if defined(TEXTURE_OPTION_TEX_ARRAY)
	//float layer;
#endif
} rp_uniforms;

oclraster_images {
	read_only image2d texture;
	// TODO: if TEXTURE_OPTION_TEX_ARRAY is defined, a 2d tex array is used -> use a special image2d type?
};

oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

void rasterization_main() {
	//const float2 screen_offset = (float2)(-(mvpm[3][0] + 1.0f), -(mvpm[3][1] + 1.0f)) / (float2)(mvpm[0][0], mvpm[1][1]);
	const float2 screen_offset = (float2)(0.0f, 0.0f); // TODO: !
	float2 interpolator_dir = (fragment_coord.xy + screen_offset - rp_uniforms->extent.xy) / (rp_uniforms->extent.zw - rp_uniforms->extent.xy);
	interpolator_dir *= (rp_uniforms->orientation.zw - rp_uniforms->orientation.xy);
	const float2 tex_coord = fmod(interpolator_dir + rp_uniforms->orientation.xy, (float2)(1.0f, 1.0f));
	
	const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
#if !defined(TEXTURE_OPTION_TEX_ARRAY)
	float4 color = image_read(texture, sampler, tex_coord);
#else
	float4 color = image_read(texture, sampler, (float3)(tex_coord, rp_uniforms->layer));
#endif
	
	// simple multiply/add color
#if defined(TEXTURE_MADD_COLOR)
	color *= rp_uniforms->mul_color;
	color += rp_uniforms->add_color;
#endif
	
	// gradients
#if defined(TEXTURE_GRADIENT)
	float4 gradient_color;
#if defined(TEXTURE_GRADIENT_HORIZONTAL)
	gradient_horizontal(gradient_color);
#elif defined(TEXTURE_GRADIENT_VERTICAL)
	gradient_vertical(gradient_color);
#elif defined(TEXTURE_GRADIENT_DIAGONAL_LR)
	gradient_diagonal_lr(gradient_color);
#elif defined(TEXTURE_GRADIENT_DIAGONAL_RL)
	gradient_diagonal_rl(gradient_color);
#endif
	
	color = mix(color * rp_uniforms->mul_color, gradient_color, rp_uniforms->gradient_mul_interpolator);
	color += rp_uniforms->add_color;
	color += gradient_color * rp_uniforms->gradient_add_interpolator;
#endif
	
	// output:
#if defined(TEXTURE_STD) || defined(TEXTURE_MADD_COLOR) || defined(TEXTURE_GRADIENT)
	framebuffer->color.xyz = linear_blend(framebuffer->color.xyz, color.xyz, color.w);
	framebuffer->color.w = color.w + (framebuffer->color.w * (1.0f - color.w));
#endif
	
#if defined(TEXTURE_PASSTHROUGH)
	framebuffer->color = color;
#endif
}

#endif
