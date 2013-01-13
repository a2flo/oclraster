
#include "oclr_global.h"

//
kernel void clear_framebuffer(const uint2 framebuffer_size,
							  write_only image2d_t color_framebuffer
#if defined(DEPTH_FRAMEBUFFER)
							  , write_only image2d_t depth_framebuffer
#endif
							  ) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= framebuffer_size.x) return;
	if(y >= framebuffer_size.y) return;
	write_imagef(color_framebuffer, (int2)(x, y), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
#if defined(DEPTH_FRAMEBUFFER)
	write_imagef(depth_framebuffer, (int2)(x, y), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
#endif
}
