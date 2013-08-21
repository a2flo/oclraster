
#include "support/gfx2d.h"

#if defined(OCLRASTER_TRANSFORM_PROGRAM)
//////////////////////////////////////////////////////////////////
// transform program
oclraster_in simple_input {
	float2 vertex;
} input_attributes;

oclraster_uniforms transform_uniforms {
	mat4 modelview_matrix;
} tp_uniforms;

float4 gfx2d_transform() {
	return mat4_mul_vec4(tp_uniforms->modelview_matrix,
						 (float4)(input_attributes->vertex, 0.0f, 1.0f));
}

#elif defined(OCLRASTER_RASTERIZATION_PROGRAM)
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_uniforms rasterize_uniforms {
	float4 gradients[4]; // -> max gradient colors: 4
	float4 stops;
	float4 extent;
} rp_uniforms;

oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

bool gfx2d_rasterization() {
	float4 color;
#if defined(GRADIENT_HORIZONTAL)
	gradient_horizontal(color);
#elif defined(GRADIENT_VERTICAL)
	gradient_vertical(color);
#elif defined(GRADIENT_DIAGONAL_LR)
	gradient_diagonal_lr(color);
#elif defined(GRADIENT_DIAGONAL_RL)
	gradient_diagonal_rl(color);
#endif
	
	framebuffer->color.xyz = linear_blend(framebuffer->color.xyz, color.xyz, (float3)(color.w));
	framebuffer->color.w = color.w + (framebuffer->color.w * (1.0f - color.w));
	
	return true;
}
#endif
