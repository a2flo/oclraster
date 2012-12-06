
#include "oclr_global.h"

//
kernel void clear_framebuffer(write_only image2d_t framebuffer) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= get_global_size(0)) return;
	if(y >= get_global_size(1)) return;
	write_imagef(framebuffer, (int2)(x, y), (float4)(0.0f, 0.0f, 0.0f, 0.0f));
}
