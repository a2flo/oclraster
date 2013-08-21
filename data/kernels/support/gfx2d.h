
#ifndef __OCLRASTER_SUPPORT_GFX2D_H__
#define __OCLRASTER_SUPPORT_GFX2D_H__

// recommended for internal use only!
#define linear_blend(v0, v1, interp) (mad(v1 - v0, interp, v0))

// gradient computation
#define compute_gradient(dst_color, interpolator_) { \
	const float interpolator = interpolator_; \
	dst_color = (float4)(0.0f); \
	/* built-in step function can not be trusted -> branch instead ... */ \
	if(interpolator < rp_uniforms->stops.y) { \
		const float interp = smoothstep(rp_uniforms->stops.x, rp_uniforms->stops.y, interpolator); \
		dst_color += linear_blend(rp_uniforms->gradients[0], rp_uniforms->gradients[1], interp); \
	} \
	else if(interpolator < rp_uniforms->stops.z) { \
		const float interp = smoothstep(rp_uniforms->stops.y, rp_uniforms->stops.z, interpolator); \
		dst_color += linear_blend(rp_uniforms->gradients[1], rp_uniforms->gradients[2], interp); \
	} \
	else if(interpolator <= rp_uniforms->stops.w) { \
		const float interp = smoothstep(rp_uniforms->stops.z, rp_uniforms->stops.w, interpolator); \
		dst_color += linear_blend(rp_uniforms->gradients[2], rp_uniforms->gradients[3], interp); \
	} \
}

#define gradient_horizontal(dst_color) { \
	compute_gradient(dst_color, \
					 (fragment_coord.x - rp_uniforms->extent.x) / \
					 (rp_uniforms->extent.z - rp_uniforms->extent.x)); \
}

#define gradient_vertical(dst_color) { \
	compute_gradient(dst_color, \
					 (fragment_coord.y - rp_uniforms->extent.y) / \
					 (rp_uniforms->extent.w - rp_uniforms->extent.y)); \
}

#define gradient_diagonal_lr(dst_color) { \
	const float2 interpolator_dir = ((fragment_coord.xy - rp_uniforms->extent.xy) / \
									 (rp_uniforms->extent.zw - rp_uniforms->extent.xy)); \
	compute_gradient(dst_color, (interpolator_dir.x + interpolator_dir.y) * 0.5f); \
}

#define gradient_diagonal_rl(dst_color) { \
	const float2 interpolator_dir = ((fragment_coord.xy - rp_uniforms->extent.xy) / \
									 (rp_uniforms->extent.zw - rp_uniforms->extent.xy)); \
	compute_gradient(dst_color,  ((1.0f - interpolator_dir.x) + interpolator_dir.y) * 0.5f); \
}

#endif
