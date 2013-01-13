
#include "oclr_global.h"

typedef struct __attribute__((packed)) {
	unsigned int triangle_count;
} constant_data;

typedef struct __attribute__((packed)) {
	float4 VV0;
	float4 VV1;
	float4 VV2;
	float4 W;
} transformed_data;

//
kernel void rasterize(global const transformed_data* transformed_buffer,
					  global const unsigned int* triangle_queues_buffer,
					  global const unsigned int* queue_sizes_buffer,
					  const uint2 tile_size,
					  const uint2 bin_count,
					  const unsigned int queue_size,
					  constant constant_data* cdata,
					  const uint2 framebuffer_size,
					  write_only image2d_t color_framebuffer,
					  write_only image2d_t depth_framebuffer) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= framebuffer_size.x) return;
	if(y >= framebuffer_size.y) return;
	
	float4 pixel_color = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	
	const float far_plane = 1000.0f; // TODO: get this from somewhere else
	float pixel_depth = far_plane;
	
	const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
	const unsigned int queue_entries = queue_sizes_buffer[bin_index];
	const unsigned int queue_offset = (queue_size * bin_index);
	for(unsigned int queue_entry = 0; queue_entry < queue_entries; queue_entry++) {
		const unsigned int triangle_id = triangle_queues_buffer[queue_offset + queue_entry];
		const float4 VV0 = transformed_buffer[triangle_id].VV0;
		const float4 VV1 = transformed_buffer[triangle_id].VV1;
		const float4 VV2 = transformed_buffer[triangle_id].VV2;
		
		//
		const float3 xy1 = (float3)((float)x, (float)y, 1.0f);
		float4 gad = (float4)(dot(xy1, VV0.xyz),
							  dot(xy1, VV1.xyz),
							  dot(xy1, VV2.xyz),
							  VV0.w);
		if(gad.x >= 0.0f || gad.y >= 0.0f || gad.z >= 0.0f) continue;
		
		// simplified:
		gad /= gad.x + gad.y + gad.z;
		
		// depth test:
		if(gad.w >= pixel_depth) continue;
		pixel_depth = gad.w;
		
		pixel_color.xyz = gad.xyz;
		write_imagef(color_framebuffer, (int2)(x, y), pixel_color);
	}
	
#if 0
	// writes triangle density of a tile (for debugging purposes)
	const float triangle_density = clamp(convert_float(queue_entries) / 255.0f, 0.0f, 1.0f);
	//if(pixel_depth < far_plane) {
		write_imagef(color_framebuffer, (int2)(x, y),
					 (float4)(triangle_density, triangle_density, triangle_density, 1.0f));
	//}
#endif
	
	// write last depth (if it has changed)
	if(pixel_depth < far_plane) {
		write_imagef(depth_framebuffer, (int2)(x, y), (float4)(pixel_depth / far_plane, 0.0f, 0.0f, 1.0f));
	}
}
