
#include "oclr_global.h"
#include "oclr_image.h"

#if defined(IMAGE_FRAMEBUFFERS)

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
	write_imagef(depth_framebuffer, (int2)(x, y), (float4)(FLT_MAX, 0.0f, 0.0f, 0.0f));
#endif
}

#else

//
kernel void clear_framebuffer(const uint2 framebuffer_size,
							  global uchar4* color_framebuffer
#if defined(DEPTH_FRAMEBUFFER)
							  , global float* depth_framebuffer
#endif
							  ) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= framebuffer_size.x) return;
	if(y >= framebuffer_size.y) return;
	const unsigned int offset = y * framebuffer_size.x + x + OCLRASTER_IMAGE_HEADER_SIZE;
	color_framebuffer[offset] = (uchar4)(0u, 0u, 0u, 0u);
#if defined(DEPTH_FRAMEBUFFER)
	depth_framebuffer[offset] = FLT_MAX;
#endif
}

#endif
