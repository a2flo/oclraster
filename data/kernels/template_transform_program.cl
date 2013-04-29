	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	#include "oclr_image.h"
	
	typedef struct __attribute__((packed, aligned(16))) {
		float4 camera_position;
		float4 camera_origin;
		float4 camera_x_vec;
		float4 camera_y_vec;
		float4 camera_forward;
		float4 frustum_normals[3];
		uint2 viewport;
	} constant_data;
	
	//
	#define discard() { return (float4)(INFINITY); }
	//###OCLRASTER_USER_CODE###

	//
	kernel void oclraster_transform(//###OCLRASTER_USER_STRUCTS###
									global float4* transformed_vertex_buffer,
									constant constant_data* cdata,
									const unsigned int vertex_count,
									const unsigned int instance_count) {
		const unsigned int global_id = get_global_id(0);
		// the global work size is greater than the actual (vertex count * instance count)
		// -> check for (vertex count * instance count) instead of get_global_size(0)
		if(global_id >= (vertex_count * instance_count)) return;
		
		const unsigned int vertex_id = global_id % vertex_count;
		const unsigned int instance_id = global_id / vertex_count;
		const unsigned int instance_vertex_id = instance_id * vertex_count + vertex_id;
		
		//
		const float3 camera_position = cdata->camera_position.xyz;
		
		//###OCLRASTER_USER_PRE_MAIN_CALL###
		const float4 user_vertex = //###OCLRASTER_USER_MAIN_CALL###
		if(user_vertex.x != INFINITY) {
			transformed_vertex_buffer[instance_vertex_id] = (float4)(user_vertex.xyz - camera_position, 1.0f);
			
			// note: this is isn't the most space efficient way to do this,
			// but it doesn't require any index -> triangle id mapping or
			// multiple dependent memory lookups (-> faster in the end)
			//###OCLRASTER_USER_OUTPUT_COPY###
		}
		else {
			transformed_vertex_buffer[instance_vertex_id] = (float4)(INFINITY);
		}
	}
