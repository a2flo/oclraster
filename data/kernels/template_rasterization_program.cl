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

	// shortcut for the opengl folks
	#define discard() { return false; }
	//###OCLRASTER_DEPTH_TEST_FUNCTION###
	//###OCLRASTER_USER_CODE###
	
	//
	kernel void oclraster_rasterization(//###OCLRASTER_USER_STRUCTS###
										
										global const unsigned int* index_buffer,
										
										global unsigned int* bin_distribution_counter,
										global const transformed_data* transformed_buffer,
										global const uchar* bin_queues,
										
										const uint2 bin_count,
										const unsigned int bin_count_lin,
										const uint2 bin_offset,
										const unsigned int batch_count,
										const unsigned int intra_bin_groups,
										
										const unsigned int primitive_type,
										const unsigned int instance_primitive_count,
										const unsigned int instance_index_count,
										
										const uint2 framebuffer_size,
										const uint4 scissor_rectangle) {
		const unsigned int local_id = get_local_id(0);
		const unsigned int local_size = get_local_size(0);
		
#if defined(CPU)
#define NO_BARRIER
#else
#define LOCAL_MEM_COPY
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
		#define LOCAL_MEM_BATCH_COUNT ((LOCAL_MEM_SIZE / BATCH_SIZE) - 1)
		//#define LOCAL_MEM_BATCH_COUNT 32u
		local uchar primitive_queue[LOCAL_MEM_BATCH_COUNT * BATCH_SIZE] __attribute__((aligned(16)));
		unsigned int triangle_offsets[LOCAL_MEM_BATCH_COUNT]; // stores the triangle id offsets for valid batches
		event_t events[LOCAL_MEM_BATCH_COUNT];
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
		const unsigned int valid_batch_count = batch_count;
		{
#endif
			
#if defined(LOCAL_MEM_COPY)
			// only read batches into local memory when they're non-empty
			// note that this doesn't require any synchronization, since it's the same for all work-items
			unsigned int valid_batch_count = 0;
			size_t batch_offset = (bin_idx * batch_count) * BATCH_SIZE;
			for(unsigned int batch_idx = 0; batch_idx < batch_count; batch_idx++, batch_offset += BATCH_SIZE) {
				if(bin_queues[batch_offset] == 0xFF && bin_queues[batch_offset + 1] == 0xFF) {
					continue;
				}
				
				events[valid_batch_count] = async_work_group_copy(&primitive_queue[valid_batch_count * BATCH_SIZE],
																  (global const uchar*)(bin_queues + batch_offset),
																  BATCH_SIZE, 0);
				triangle_offsets[valid_batch_count] = batch_idx * BATCH_SIZE;
				valid_batch_count++;
			}
			
			// early-out when there are no valid batches
			if(valid_batch_count == 0) continue;
			
			// since we're not immediately waiting on all batch copies to finish, wait here
			for(unsigned int batch_idx = 0; batch_idx < valid_batch_count; batch_idx++) {
				wait_group_events(1, &events[batch_idx]);
			}
#else
			const size_t global_queue_offset = (bin_idx * batch_count) * BATCH_SIZE;
#endif
			
			//
			const uint2 bin_location = (uint2)(bin_idx % bin_count.x, bin_idx / bin_count.x) + bin_offset;
			for(unsigned int i = 0; i < intra_bin_groups; i++) {
				const unsigned int fragment_idx = (i * local_size) + local_id;
				const uint2 local_xy = (uint2)(fragment_idx % BIN_SIZE, fragment_idx / BIN_SIZE);
				if(local_xy.y >= BIN_SIZE) continue;
				const unsigned int x = bin_location.x * BIN_SIZE + local_xy.x;
				const unsigned int y = bin_location.y * BIN_SIZE + local_xy.y;
				const float2 fragment_coord = (float2)(x, y) + 0.5f;
				if(x >= framebuffer_size.x || y >= framebuffer_size.y ||
				   x < scissor_rectangle.x || x > scissor_rectangle.z ||
				   y < scissor_rectangle.y || y > scissor_rectangle.w) {
					continue;
				}
				
				//###OCLRASTER_FRAMEBUFFER_READ###
				
				// simple counter/flag that signals if fragments have passed
				// (actual value doesn't matter, only if it's 0.0f or not)
				float fragments_passed = 0.0f;
				
				//
				for(unsigned int batch_idx = 0, queue_offset = 0;
					batch_idx < valid_batch_count;
					batch_idx++, queue_offset += BATCH_SIZE) {
#if defined(LOCAL_MEM_COPY)
					local const uchar* queue_ptr = &primitive_queue[queue_offset];
#else
					global const uchar* queue_ptr = &bin_queues[global_queue_offset + queue_offset];
					
					// check if queue is empty
					if(queue_ptr[0] == 0xFF && queue_ptr[1] == 0xFF) {
						continue;
					}
#endif
					
					//
					for(unsigned int idx = 0; idx < BATCH_SIZE; idx++) {
						const unsigned int queue_data = queue_ptr[idx];
						if(queue_data < idx) break; // end of queue
#if defined(LOCAL_MEM_COPY)
						const unsigned int primitive_id = triangle_offsets[batch_idx] + queue_data;
#else
						const unsigned int primitive_id = queue_offset + queue_data;
#endif
						const unsigned int instance_id = primitive_id / instance_primitive_count;
						
						//
						{
							const float3 VV0 = (float3)(transformed_buffer[primitive_id].data[0],
														transformed_buffer[primitive_id].data[1],
														transformed_buffer[primitive_id].data[2]);
							const float3 VV1 = (float3)(transformed_buffer[primitive_id].data[3],
														transformed_buffer[primitive_id].data[4],
														transformed_buffer[primitive_id].data[5]);
							const float3 VV2 = (float3)(transformed_buffer[primitive_id].data[6],
														transformed_buffer[primitive_id].data[7],
														transformed_buffer[primitive_id].data[8]);
							
							//
							float4 barycentric = (float4)(mad(fragment_coord.x, VV0.x, mad(fragment_coord.y, VV0.y, VV0.z)),
														  mad(fragment_coord.x, VV1.x, mad(fragment_coord.y, VV1.y, VV1.z)),
														  mad(fragment_coord.x, VV2.x, mad(fragment_coord.y, VV2.y, VV2.z)),
														  transformed_buffer[primitive_id].data[9]); // .w = computed depth
							
#if defined(OCLRASTER_PROJECTION_PERSPECTIVE)
							if(barycentric.x >= 0.0f || barycentric.y >= 0.0f || barycentric.z >= 0.0f) continue;
#elif defined(OCLRASTER_PROJECTION_ORTHOGRAPHIC)
#define BARYCENTRIC_EPSILON 0.00001f
							// this is sadly necessary, due to fp imprecision (this proved to be the most stable/consistent solution)
							barycentric.xyz = select(barycentric.xyz, (float3)(0.0f),
													 isless(fabs(barycentric.xyz), (float3)(BARYCENTRIC_EPSILON)));
							
							// general case: completely outside the primitive
							if(barycentric.x < 0.0f || barycentric.y < 0.0f || barycentric.z < 0.0f) continue;
							
							// "consistency rules" (fragment is on the edge of a primitive or on a vertex):
							// -> at least one barycentrix element "i" is 0
							// -> valid fragment if: VVi.x must be > 0 or VVi.x must be == 0 and VVi.y must be < 0
							if(barycentric.x == 0.0f) {
								if(VV0.x < 0.0f) continue;
								else if(VV0.x == 0.0f && VV0.y >= 0.0f) continue;
							}
							if(barycentric.y == 0.0f) {
								if(VV1.x < 0.0f) continue;
								else if(VV1.x == 0.0f && VV1.y >= 0.0f) continue;
							}
							if(barycentric.z == 0.0f) {
								if(VV2.x < 0.0f) continue;
								else if(VV2.x == 0.0f && VV2.y >= 0.0f) continue;
							}
#endif
							
							// simplified:
							barycentric /= barycentric.x + barycentric.y + barycentric.z;
							
							// ignore fragments with negative depth
							if(barycentric.w < 0.0f) continue;
							
#if !defined(OCLRASTER_NO_DEPTH) && !defined(OCLRASTER_NO_DEPTH_TEST)
#if !defined(OCLRASTER_DEPTH_OVERRIDE)
							// early depth test
							if(!depth_test(barycentric.w, *fragment_depth)) continue;
#else
							// need to save the old depth value if the user overwrites the framebuffer depth
							const float prev_depth = *fragment_depth;
#endif
#endif
							
							// note: if a fragment is discarded, this will "continue"
							// -> depth is not updated and fragment counter is not increased
							//###OCLRASTER_USER_MAIN_CALL###
							
#if !defined(OCLRASTER_NO_DEPTH) && !defined(OCLRASTER_NO_DEPTH_TEST)
#if !defined(OCLRASTER_DEPTH_OVERRIDE)
							// set framebuffer depth for this fragment (-> user doesn't set it)
							*fragment_depth = barycentric.w;
#else
							// depth test when "depth-override" is active, i.e. the depth is written by the user program
							if(!depth_test(*fragment_depth, prev_depth)) {
								*fragment_depth = prev_depth; // restore previous depth value
								continue;
							}
#endif
#endif
							
							fragments_passed += 1.0f;
						}
					}
				}
				
				// write framebuffer output (if any fragment has passed)
				if(fragments_passed != 0.0f) {
					//###OCLRASTER_FRAMEBUFFER_WRITE###
				}
			}
		}
	}
