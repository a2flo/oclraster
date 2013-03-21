	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	#include "oclr_image.h"
	
	typedef struct __attribute__((packed, aligned(16))) {
		// VV0: 0 - 2
		// VV1: 3 - 5
		// VV2: 6 - 8
		// depth: 9
		// unused: 10 - 11
		// x_bounds: 12 - 13 (.x/12 = INFINITY if culled)
		// y_bounds: 14 - 15
		const float data[16];
	} transformed_data;
	
	//###OCLRASTER_USER_CODE###
	
	//
	#define BIN_SIZE 64u
	#define BATCH_SIZE 256u
	kernel void oclraster_program(//###OCLRASTER_USER_STRUCTS###
								  
								  global unsigned int* bin_distribution_counter,
								  global const transformed_data* transformed_buffer,
								  global const ulong16* bin_queues,
								  
								  const uint2 bin_count,
								  const unsigned int bin_count_lin,
								  const unsigned int batch_count,
								  const unsigned int batch_size,
								  
								  const uint2 framebuffer_size) {
		/*const unsigned int x = get_global_id(0);
		const unsigned int y = get_global_id(1);
		if(x >= framebuffer_size.x) return;
		if(y >= framebuffer_size.y) return;*/
		
		const unsigned int global_id = get_global_id(0);
		const unsigned int local_id = get_local_id(0);
		
		// init counter
		if(global_id == 0) {
			*bin_distribution_counter = 0;
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
		
		//
		local unsigned int bin_idx;
		local uchar triangle_queue[BATCH_SIZE] __attribute__((aligned(8)));
		for(;;) {
			// get next bin index
			if(local_id == 0) {
				// only done once per work-group (-> only work-item #0)
				bin_idx = atomic_inc(bin_distribution_counter);
			}
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// check if all bins have been processed
			if(bin_idx >= bin_count_lin) {
				return;
			}
			
			//
			for(unsigned int batch_idx = 0; batch_idx < batch_count; batch_idx++) {
				const size_t offset = (bin_count_lin * batch_idx + bin_idx) * (BATCH_SIZE / 128u);
				event_t event = async_work_group_copy(&triangle_queue[0],
													  (global const uchar*)(bin_queues + offset),
													  BATCH_SIZE, 0);
				wait_group_events(1, &event);
			}
		}
		
#if 0
		// TODO: handling if there is no depth buffer / depth testing
		// TODO: stencil testing
		// TODO: scissor testing
		
		//###OCLRASTER_FRAMEBUFFER_READ###
		
		const float2 fragment_coord = (float2)(x, y);
		const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
		const unsigned int queue_entries = queue_sizes_buffer[bin_index];
		const unsigned int queue_offset = (queue_size * bin_index);
		//if(queue_entries > 0) framebuffer.color = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
		for(unsigned int queue_entry = 0; queue_entry < queue_entries; queue_entry++) {
			
			const unsigned int triangle_id = triangle_queues_buffer[queue_offset + queue_entry];
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
#endif
	}
