/*
 *  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "transform_program.h"

// awesome raw string literals are awesome
static constexpr char template_transform_program[] { u8R"OCLRASTER_RAWSTR(
	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	
	typedef struct __attribute__((packed)) {
		float4 camera_position;
		float4 camera_origin;
		float4 camera_x_vec;
		float4 camera_y_vec;
		float4 camera_forward;
		uint2 viewport;
	} constant_data;
	
	typedef struct __attribute__((packed)) {
		float4 VV0;
		float4 VV1;
		float4 VV2;
		float4 W;
	} transformed_data;
	
	// transform rerouting
	#define transform(vertex) _oclraster_transform(vertex, VE, transformed_vertex)
	OCLRASTER_FUNC void _oclraster_transform(float4 vertex, const float3* VE, float3* transformed_vertex) {
		*transformed_vertex = vertex.xyz - *VE;
	}
	
	//###OCLRASTER_USER_CODE###
	
	//
	kernel void _oclraster_program(//###OCLRASTER_USER_STRUCTS###
								   global const unsigned int* index_buffer,
								   global transformed_data* transformed_buffer,
								   constant constant_data* cdata,
								   const unsigned int triangle_count) {
		const unsigned int triangle_id = get_global_id(0);
		// global work size is greater than the actual triangle count
		// -> check for triangle_count instead of get_global_size(0)
		if(triangle_id >= triangle_count) return;
		
		const unsigned int indices[3] = {
			index_buffer[triangle_id*3],
			index_buffer[triangle_id*3 + 1],
			index_buffer[triangle_id*3 + 2]
		};
		
		//
		const float3 D0 = cdata->camera_origin.xyz;
		const float3 DX = cdata->camera_x_vec.xyz;
		const float3 DY = cdata->camera_y_vec.xyz;
		const float3 VE = cdata->camera_position.xyz;
		const float3 forward = cdata->camera_forward.xyz;
		
		float3 vertices[3];
		//###OCLRASTER_USER_PRE_MAIN_CALL###
		for(int i = 0; i < 3; i++) {
			//###OCLRASTER_USER_MAIN_CALL###
		}
		
		// if component < 0 => vertex is behind cam, == 0 => on the near plane, > 0 => in front of the cam
		const float4 triangle_cam_relation = (float4)(dot(vertices[0], forward),
													  dot(vertices[1], forward),
													  dot(vertices[2], forward),
													  0.0f);
		// TODO: if xyz < 0, don't add it in the first place
		
		// TODO: actual culling
		
		// since VE0 can be interpreted as (0, 0, 0, 1) after it has been substracted from the vertices,
		// the original algorithm (requiring the computation of 4x4 matrix determinants) can be simplified:
		const float3 c01 = cross(vertices[0], vertices[1]);
		const float3 c20 = cross(vertices[2], vertices[0]);
		const float3 c12 = cross(vertices[1], vertices[2]);
		const float o01 = dot(D0, c01);
		const float o20 = dot(D0, c20);
		const float o12 = dot(D0, c12);
		const float x01 = dot(DX, c01);
		const float x20 = dot(DX, c20);
		const float x12 = dot(DX, c12);
		const float y01 = dot(DY, c01);
		const float y20 = dot(DY, c20);
		const float y12 = dot(DY, c12);
		
		float4 VV0 = (float4)(x12, y12, o12, dot(vertices[0], c12));
		float4 VV1 = (float4)(x20, y20, o20, 0.0f);
		float4 VV2 = (float4)(x01, y01, o01, 0.0f);
		
		//
		transformed_buffer[triangle_id].VV0 = VV0;
		transformed_buffer[triangle_id].VV1 = VV1;
		transformed_buffer[triangle_id].VV2 = VV2;
		transformed_buffer[triangle_id].W = triangle_cam_relation; // TODO: don't call it .W any more
	}
)OCLRASTER_RAWSTR"};

transform_program::transform_program(const string& code, const string& identifier_, const string entry_function_) :
oclraster_program(code, identifier_, entry_function_) {
	process_program(code);
}

transform_program::~transform_program() {
}

void transform_program::specialized_processing(const string& code) {
	// insert (processed) user code into template program
	program_code = template_transform_program;
	core::find_and_replace(program_code, "//###OCLRASTER_USER_CODE###", code);
	
	// create kernel parameters string (buffers will be called "user_buffer_#")
	string kernel_parameters = "";
	size_t user_buffer_count = 0;
	for(const auto& oclr_struct : structs) {
		switch (oclr_struct.type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				kernel_parameters += "global const ";
				break;
			case oclraster_program::STRUCT_TYPE::OUTPUT:
				kernel_parameters += "global ";
				break;
			case oclraster_program::STRUCT_TYPE::UNIFORMS:
				kernel_parameters += "constant ";
				break;
		}
		kernel_parameters += oclr_struct.name + "* user_buffer_"+size_t2string(user_buffer_count)+",\n";
		user_buffer_count++;
	}
	core::find_and_replace(program_code, "//###OCLRASTER_USER_STRUCTS###", kernel_parameters);
	
	// insert main call + prior buffer handling
	string buffer_handling_code = "";
	string pre_buffer_handling_code = "";
	string main_call_parameters = "";
	size_t cur_user_buffer = 0;
	for(const auto& oclr_struct : structs) {
		const string cur_user_buffer_str = size_t2string(cur_user_buffer);
		switch (oclr_struct.type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				buffer_handling_code += (oclr_struct.name + " user_buffer_element_" + cur_user_buffer_str +
										 " = user_buffer_"+cur_user_buffer_str+"[indices[i]];\n");
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
			case oclraster_program::STRUCT_TYPE::OUTPUT:
				buffer_handling_code += ("global " + oclr_struct.name + "* user_buffer_element_" + cur_user_buffer_str +
										 " = &user_buffer_"+cur_user_buffer_str+"[indices[i]];\n");
				main_call_parameters += "user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
			case oclraster_program::STRUCT_TYPE::UNIFORMS:
				pre_buffer_handling_code += ("const " + oclr_struct.name + " user_buffer_element_" +
											 cur_user_buffer_str + " = *user_buffer_" + cur_user_buffer_str + ";\n");
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
		}
		cur_user_buffer++;
	}
	main_call_parameters += "i, &VE, &vertices[i]"; // the same for all transform programs
	core::find_and_replace(program_code, "//###OCLRASTER_USER_PRE_MAIN_CALL###", pre_buffer_handling_code);
	core::find_and_replace(program_code, "//###OCLRASTER_USER_MAIN_CALL###",
						   buffer_handling_code+"_oclraster_user_"+entry_function+"("+main_call_parameters+");");
	
	// done
	//oclr_msg("generated transform user program: %s", program_code);
}

string transform_program::create_entry_function_parameters() {
	static const string transform_params = "const int index, const float3* VE, float3* transformed_vertex";
	string entry_function_params = "";
	for(size_t i = 0, struct_count = structs.size(); i < struct_count; i++) {
		switch(structs[i].type) {
			case STRUCT_TYPE::INPUT:
			case STRUCT_TYPE::UNIFORMS:
				entry_function_params += "const ";
				break;
			case STRUCT_TYPE::OUTPUT:
				entry_function_params += "global ";
				break;
		}
		entry_function_params += structs[i].name + "* " + structs[i].object_name;
		if(i < struct_count-1) entry_function_params += ", ";
	}
	if(entry_function_params != "") entry_function_params += ", ";
	entry_function_params += transform_params;
	return entry_function_params;
}
