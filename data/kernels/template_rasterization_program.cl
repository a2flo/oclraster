	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	#include "oclr_image.h"
	#include "oclr_primitive_assembly.h"

	typedef struct __attribute__((packed, aligned(4))) {
		// VV0: 0 - 2
		// VV1: 3 - 5
		// VV2: 6 - 8
		// depth: 9
		float data[10];
	} transformed_data;

	//###OCLRASTER_USER_CODE###
	
	//
	kernel void oclraster_rasterization(//###OCLRASTER_USER_STRUCTS###
										
										global const unsigned int* index_buffer,
										
										global unsigned int* bin_distribution_counter,
										global const transformed_data* transformed_buffer,
										global const uchar* bin_queues,
										
										const uint2 bin_count,
										const unsigned int bin_count_lin,
										const unsigned int batch_count,
										const unsigned int intra_bin_groups,
										
										const unsigned int primitive_type,
										const uint2 framebuffer_size) {
		const unsigned int local_id = get_local_id(0);
		const unsigned int local_size = get_local_size(0);
		
#if defined(CPU)
#define NO_BARRIER 1
#else
#define LOCAL_MEM_COPY 1
#endif
		
#if !defined(NO_BARRIER)
		const unsigned int global_id = get_global_id(0);
		// init counter
		if(global_id == 0) {
			*bin_distribution_counter = 0;
		}
		barrier(CLK_GLOBAL_MEM_FENCE);
#endif
		
#if defined(LOCAL_MEM_COPY)
		// -1 b/c the local memory is also used for other things
		//#define LOCAL_MEM_BATCH_COUNT ((LOCAL_MEM_SIZE / BATCH_SIZE) - 1)
		#define LOCAL_MEM_BATCH_COUNT 32u
		local uchar triangle_queue[LOCAL_MEM_BATCH_COUNT * BATCH_SIZE] __attribute__((aligned(16)));
#endif
		
#if !defined(NO_BARRIER)
		local unsigned int bin_idx;
		for(;;) {
			// get next bin index
			// note that this barrier is necessary, because not all work-items are running this kernel synchronously
			barrier(CLK_LOCAL_MEM_FENCE);
			if(local_id == 0) {
				// only done once per work-group (-> only work-item #0)
				bin_idx = atomic_inc(bin_distribution_counter);
			}
			barrier(CLK_LOCAL_MEM_FENCE);
			
			// check if all bins have been processed
			if(bin_idx >= bin_count_lin) {
				return;
			}
#else
		const unsigned int bin_idx = get_group_id(0);
		{
#endif
			
#if defined(LOCAL_MEM_COPY)
			// read all batches into local memory at once
			const size_t offset = (bin_idx * batch_count) * BATCH_SIZE;
			event_t event = async_work_group_copy(&triangle_queue[0],
												  (global const uchar*)(bin_queues + offset),
												  batch_count * BATCH_SIZE, 0);
			wait_group_events(1, &event);
#else
			const size_t global_queue_offset = (bin_idx * batch_count) * BATCH_SIZE;
#endif
			
			//
			const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x);
			for(unsigned int i = 0; i < intra_bin_groups; i++) {
				const unsigned int fragment_idx = (i * local_size) + local_id;
				const uint2 local_xy = (uint2)(fragment_idx % BIN_SIZE, fragment_idx / BIN_SIZE);
				const unsigned int x = bin_location.x * BIN_SIZE + local_xy.x;
				const unsigned int y = bin_location.y * BIN_SIZE + local_xy.y;
				const float2 fragment_coord = (float2)(x, y);
				if(x >= framebuffer_size.x || y >= framebuffer_size.y) continue;
				
				// TODO: handling if there is no depth buffer / depth testing
				// TODO: stencil testing
				// TODO: scissor testing
				
				//###OCLRASTER_FRAMEBUFFER_READ###
				
				//if(queue_entries > 0) framebuffer.color = (float4)(1.0f, 1.0f, 1.0f, 1.0f);
				
				//
				for(unsigned int batch_idx = 0, queue_offset = 0;
					batch_idx < batch_count;
					batch_idx++, queue_offset += BATCH_SIZE) {
#if defined(LOCAL_MEM_COPY)
					local const uchar* queue_ptr = &triangle_queue[queue_offset];
#else
					global const uchar* queue_ptr = &bin_queues[global_queue_offset + queue_offset];
#endif
					// check if queue is empty
					if(queue_ptr[0] == 0xFF && queue_ptr[1] == 0xFF) {
						continue;
					}
					
					//
					unsigned int last_id = 0;
					for(unsigned int idx = 0; idx < BATCH_SIZE; idx++) {
						const unsigned int triangle_id = queue_offset + queue_ptr[idx];
						if(triangle_id < last_id) break; // end of queue
						last_id = triangle_id;
						
						//
						{
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
							if(barycentric.w < 0.0f || barycentric.w >= *fragment_depth) continue;
							
							// reset depth (note: fragment_color will contain the last valid color)
							*fragment_depth = barycentric.w;
							
							//###OCLRASTER_USER_MAIN_CALL###
						}
					}
					
					// write framebuffer output (if depth has changed)
					if(*fragment_depth < input_depth || false) {
						//###OCLRASTER_FRAMEBUFFER_WRITE###
					}
				}
			}
		}
	}
