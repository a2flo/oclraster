	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	#include "oclr_image.h"
	
	typedef struct __attribute__((packed)) {
		unsigned int triangle_count;
	} constant_data;
	
	typedef struct __attribute__((packed, aligned(16))) {
		// VV0: 0 - 2
		// VV1: 3 - 5
		// VV2: 6 - 8
		// depth: 9
		// cam relation: 10 - 12
		// unused: 13 - 14
		// 15: culled flag (0: valid; 1: culled)
		float data[15];
		unsigned int culled;
	} transformed_data;
	
	//###OCLRASTER_USER_CODE###
	
	//
	kernel void _oclraster_program(//###OCLRASTER_USER_STRUCTS###
								   
								   global const transformed_data* transformed_buffer,
								   global const unsigned int* triangle_queues_buffer,
								   global const unsigned int* queue_sizes_buffer,
								   const uint2 tile_size,
								   const uint2 bin_count,
								   const unsigned int queue_size,
								   constant constant_data* cdata,
								   const uint2 framebuffer_size) {
		const unsigned int x = get_global_id(0);
		const unsigned int y = get_global_id(1);
		if(x >= framebuffer_size.x) return;
		if(y >= framebuffer_size.y) return;
		
		// TODO: handling if there is no depth buffer / depth testing
		// TODO: stencil testing
		// TODO: scissor testing
		
		//###OCLRASTER_FRAMEBUFFER_READ###
		
		const float2 fragment_coord = (float2)(x, y);
		const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
		const unsigned int queue_entries = queue_sizes_buffer[bin_index];
		const unsigned int queue_offset = (queue_size * bin_index);
		unsigned int next_id = 0xFFFFFFu, last_id = 0xFFFFFFu;
		//if(queue_entries > 0) framebuffer.color = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
		for(unsigned int queue_entry = 0; queue_entry < queue_entries; queue_entry++) {
			for(unsigned int idx = 0; idx < queue_entries; idx++) {
				const unsigned int qidx = triangle_queues_buffer[queue_offset + idx];
				if(qidx < next_id && (qidx > last_id || last_id == 0xFFFFFFu)) {
					next_id = qidx;
				}
			}
			const unsigned int triangle_id = next_id;
			last_id = next_id;
			next_id = 0xFFFFFFu;
			
			//const unsigned int triangle_id = triangle_queues_buffer[queue_offset + queue_entry];
			const float3 VV0 = (float3)(transformed_buffer[triangle_id].data[0],
										transformed_buffer[triangle_id].data[1],
										transformed_buffer[triangle_id].data[2]);
			const float3 VV1 = (float3)(transformed_buffer[triangle_id].data[3],
										transformed_buffer[triangle_id].data[4],
										transformed_buffer[triangle_id].data[5]);
			const float3 VV2 = (float3)(transformed_buffer[triangle_id].data[6],
										transformed_buffer[triangle_id].data[7],
										transformed_buffer[triangle_id].data[8]);
			
			//
			float4 barycentric = (float4)(mad(fragment_coord.x, VV0.x, mad(fragment_coord.y, VV0.y, VV0.z)),
										  mad(fragment_coord.x, VV1.x, mad(fragment_coord.y, VV1.y, VV1.z)),
										  mad(fragment_coord.x, VV2.x, mad(fragment_coord.y, VV2.y, VV2.z)),
										  transformed_buffer[triangle_id].data[9]); // .w = computed depth
			if(barycentric.x >= 0.0f || barycentric.y >= 0.0f || barycentric.z >= 0.0f) continue;
			
			// simplified:
			barycentric /= barycentric.x + barycentric.y + barycentric.z;
			
			// depth test + ignore negative depth:
			if(barycentric.w < 0.0f ||
			   barycentric.w >= *fragment_depth) continue;
			
			// reset depth (note: fragment_color will contain the last valid color)
			*fragment_depth = barycentric.w;
			
			//
			//###OCLRASTER_USER_MAIN_CALL###
		}
		
		// write framebuffer output (if depth has changed)
		if(*fragment_depth < input_depth /*|| *fragment_depth == input_depth*/) {
			//###OCLRASTER_FRAMEBUFFER_WRITE###
		}
	}
