
#include "oclr_global.h"

#define OCLRASTER_IMAGE_UCHAR4
#include "oclr_image.h"

//
kernel void framebuffer_luma(//###OCLRASTER_FRAMEBUFFER_IMAGES###
							 global uchar4* framebuffer,
							 const uint2 framebuffer_size) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= framebuffer_size.x || y >= framebuffer_size.y) {
		return;
	}
	
	const uint2 coord = (uint2)(x, y);
	const oclr_sampler_t point_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;
	const float3 color = image_read(framebuffer, point_sampler, coord).xyz;
	const float luma = color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722;
	image_write(framebuffer, coord, (float4)(color, luma));
}
