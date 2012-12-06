
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
					  write_only image2d_t framebuffer) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= get_global_size(0)) return;
	if(y >= get_global_size(1)) return;
	
	float4 pixel_color = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	
	const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
	const unsigned int queue_entries = queue_sizes_buffer[bin_index];
	/*if(queue_entries > 0) {
		pixel_color.xyz = (float3)(1.0f, 1.0f, 1.0f);
	}*/
	
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
		pixel_color.xyz = gad.xyz;
		write_imagef(framebuffer, (int2)(x, y), pixel_color);
	}
}
