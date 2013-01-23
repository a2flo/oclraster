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

#include "rasterize_program.h"

static constexpr char template_rasterize_program[] { u8R"OCLRASTER_RAWSTR(
	#include "oclr_global.h"
	#include "oclr_math.h"
	#include "oclr_matrix.h"
	
	typedef struct __attribute__((packed)) {
		unsigned int triangle_count;
	} constant_data;
	
	typedef struct __attribute__((packed)) {
		float4 VV0;
		float4 VV1;
		float4 VV2;
		float4 W;
	} transformed_data;
	
	//###OCLRASTER_USER_CODE###
	
	//
	kernel void _oclraster_program(//###OCLRASTER_USER_STRUCTS###
								   global const unsigned int* index_buffer, // TODO: necessary for now, output should be packed later on
								   
								   global const transformed_data* transformed_buffer,
								   global const unsigned int* triangle_queues_buffer,
								   global const unsigned int* queue_sizes_buffer,
								   const uint2 tile_size,
								   const uint2 bin_count,
								   const unsigned int queue_size,
								   constant constant_data* cdata,
								   const uint2 framebuffer_size,
								   global uchar4* color_framebuffer,
								   global float* depth_framebuffer) {
		const unsigned int x = get_global_id(0);
		const unsigned int y = get_global_id(1);
		if(x >= framebuffer_size.x) return;
		if(y >= framebuffer_size.y) return;
		
		const unsigned int framebuffer_offset = (y * framebuffer_size.x) + x;
		float4 pixel_color = convert_float4(color_framebuffer[framebuffer_offset]) / 255.0f;
		const float input_depth = depth_framebuffer[framebuffer_offset];
		float pixel_depth = input_depth;
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		
		const float2 fragment_xy = (float2)(x, y);
		const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
		const unsigned int queue_entries = queue_sizes_buffer[bin_index];
		const unsigned int queue_offset = (queue_size * bin_index);
		for(unsigned int queue_entry = 0; queue_entry < queue_entries; queue_entry++) {
			const unsigned int triangle_id = triangle_queues_buffer[queue_offset + queue_entry];
			const float4 VV0 = transformed_buffer[triangle_id].VV0;
			const float4 VV1 = transformed_buffer[triangle_id].VV1;
			const float4 VV2 = transformed_buffer[triangle_id].VV2;
			
			//
			float4 barycentric = (float4)(fragment_xy.x * VV0.x + fragment_xy.y * VV0.y + VV0.z,
										  fragment_xy.x * VV1.x + fragment_xy.y * VV1.y + VV1.z,
										  fragment_xy.x * VV2.x + fragment_xy.y * VV2.y + VV2.z,
										  VV0.w); // .w = computed depth
			if(barycentric.x >= 0.0f || barycentric.y >= 0.0f || barycentric.z >= 0.0f) continue;
			
			// simplified:
			barycentric /= barycentric.x + barycentric.y + barycentric.z;
			
			// depth test:
			if(barycentric.w >= pixel_depth) continue;
			
			// reset depth (note: pixel_color will contain the last valid color)
			pixel_depth = barycentric.w;
			
			//
			const unsigned int indices[3] = {
				index_buffer[triangle_id*3],
				index_buffer[triangle_id*3 + 1],
				index_buffer[triangle_id*3 + 2]
			};
			//###OCLRASTER_USER_MAIN_CALL###
		}
		
		// write last depth (if it has changed)
		mem_fence(CLK_GLOBAL_MEM_FENCE);
		if(pixel_depth < input_depth) {
			color_framebuffer[framebuffer_offset] = convert_uchar4_sat(pixel_color * 255.0f);
			depth_framebuffer[framebuffer_offset] = pixel_depth;
		}
	}
)OCLRASTER_RAWSTR"};

rasterize_program::rasterize_program(const string& code, const string entry_function_) :
oclraster_program(code, entry_function_) {
	process_program(code);
}

rasterize_program::~rasterize_program() {
}

void rasterize_program::specialized_processing(const string& code) {
	// insert (processed) user code into template program
	program_code = template_rasterize_program;
	core::find_and_replace(program_code, "//###OCLRASTER_USER_CODE###", code);
	
	// create kernel parameters string (buffers will be called "user_buffer_#")
	string kernel_parameters = "";
	size_t user_buffer_count = 0;
	for(const auto& oclr_struct : structs) {
		switch (oclr_struct.type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				// there are no input structs
				continue;
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
	string main_call_parameters = "";
	size_t cur_user_buffer = 0;
	for(const auto& oclr_struct : structs) {
		const string cur_user_buffer_str = size_t2string(cur_user_buffer);
		switch (oclr_struct.type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				// there are no input structs
				continue;
			case oclraster_program::STRUCT_TYPE::OUTPUT: {
				buffer_handling_code += "const " + oclr_struct.name + " user_buffer_outputs_" + cur_user_buffer_str + "[3] = { ";
				for(size_t i = 0; i < 3; i++) {
					buffer_handling_code += "user_buffer_" + cur_user_buffer_str + "[indices[" + size_t2string(i) + "]]";
					if(i < 2) buffer_handling_code += ", ";
				}
				buffer_handling_code += " };\n";
				
				const string interp_var_name = "interpolated_user_buffer_element_" + cur_user_buffer_str;
				buffer_handling_code += oclr_struct.name + " " + interp_var_name +";\n";
				for(const auto& var : oclr_struct.variables) {
					buffer_handling_code += interp_var_name + "." + var + " = interpolate(";
					for(size_t i = 0; i < 3; i++) {
						buffer_handling_code += "user_buffer_outputs_" + cur_user_buffer_str + "[" + size_t2string(i) + "]." + var;
						if(i < 2) buffer_handling_code += ", ";
					}
					buffer_handling_code += ", barycentric.xyz);\n";
				}
				main_call_parameters += "&" + interp_var_name + ", ";
			}
			break;
			case oclraster_program::STRUCT_TYPE::UNIFORMS:
				buffer_handling_code += ("const " + oclr_struct.name + " user_buffer_element_" +
										 cur_user_buffer_str + " = *user_buffer_" + cur_user_buffer_str + ";\n");
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
		}
		cur_user_buffer++;
	}
	main_call_parameters += "&pixel_color, &pixel_depth, fragment_xy, barycentric.xyz"; // the same for all rasterization programs
	core::find_and_replace(program_code, "//###OCLRASTER_USER_MAIN_CALL###",
						   buffer_handling_code+"_oclraster_user_"+entry_function+"("+main_call_parameters+");");
	
	// done
	//oclr_msg("generated rasterize user program: %s", program_code);
}

string rasterize_program::create_entry_function_parameters() {
	static const string rasterize_params = "float4* pixel_color, float* depth, const float2 fragment_xy, const float3 barycentric";
	string entry_function_params = "";
	for(size_t i = 0, struct_count = structs.size(); i < struct_count; i++) {
		switch(structs[i].type) {
			case STRUCT_TYPE::INPUT:
				// there are no input structs
				continue;
			case STRUCT_TYPE::UNIFORMS:
			case STRUCT_TYPE::OUTPUT:
				entry_function_params += "const ";
				break;
		}
		entry_function_params += structs[i].name + "* " + structs[i].object_name;
		if(i < struct_count-1) entry_function_params += ", ";
	}
	if(entry_function_params != "") entry_function_params += ", ";
	entry_function_params += rasterize_params;
	return entry_function_params;
}
