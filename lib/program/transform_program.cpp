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
#include "image.h"

#if defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
string template_transform_program { "" };
#else
// awesome raw string literals are awesome
static constexpr char template_transform_program[] { u8R"OCLRASTER_RAWSTR(
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
	//###OCLRASTER_USER_CODE###
	
	//
#define discard() { transformed_vertex_buffer[vertex_id] = (float4)(INFINITY); return; }
	kernel void oclraster_transform(//###OCLRASTER_USER_STRUCTS###
									global float4* transformed_vertex_buffer,
									constant constant_data* cdata,
									const unsigned int vertex_count) {
		const unsigned int vertex_id = get_global_id(0);
		// global work size is greater than the actual vertex count
		// -> check for vertex_count instead of get_global_size(0)
		if(vertex_id >= vertex_count) return;
		
		//
		const float3 camera_position = cdata->camera_position.xyz;
		
		//###OCLRASTER_USER_PRE_MAIN_CALL###
		float4 user_vertex = //###OCLRASTER_USER_MAIN_CALL###
		user_vertex.xyz -= camera_position;
		transformed_vertex_buffer[vertex_id] = user_vertex;
		
		// note: this is isn't the most space efficient way to do this,
		// but it doesn't require any index -> triangle id mapping or
		// multiple dependent memory lookups (-> faster in the end)
		//###OCLRASTER_USER_OUTPUT_COPY###
	}
)OCLRASTER_RAWSTR"};
#endif

transform_program::transform_program(const string& code, const string entry_function_, const string build_options_) :
oclraster_program(code, entry_function_, "-DOCLRASTER_TRANSFORM_PROGRAM "+build_options_) {
	kernel_function_name = "oclraster_transform";
	process_program(code);
}

transform_program::~transform_program() {
}

string transform_program::specialized_processing(const string& code,
												 const kernel_image_spec& image_spec) {
	// insert (processed) user code into template program
	string program_code = template_transform_program;
	core::find_and_replace(program_code, "//###OCLRASTER_USER_CODE###", code);
	
	//
	vector<string> image_decls;
	const string kernel_parameters { create_user_kernel_parameters(image_spec, image_decls, false) };
	core::find_and_replace(program_code, "//###OCLRASTER_USER_STRUCTS###", kernel_parameters);
	
	// insert main call + prior buffer handling
	string buffer_handling_code = "";
	string output_handling_code = "";
	string main_call_parameters = "";
	size_t cur_user_buffer = 0;
	for(const auto& oclr_struct : structs) {
		const string cur_user_buffer_str = size_t2string(cur_user_buffer);
		switch (oclr_struct->type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				buffer_handling_code += oclr_struct->name + " user_buffer_element_" + cur_user_buffer_str +
										 " = user_buffer_"+cur_user_buffer_str+"[vertex_id];\n";
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
			case oclraster_program::STRUCT_TYPE::OUTPUT:
				buffer_handling_code += oclr_struct->name + " user_buffer_element_" + cur_user_buffer_str + ";\n";
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				for(const auto& var : oclr_struct->variables) {
					output_handling_code += "user_buffer_" + cur_user_buffer_str + "[vertex_id]." + var + " = ";
					output_handling_code += "user_buffer_element_" + cur_user_buffer_str + "." + var + ";\n";
				}
				break;
			case oclraster_program::STRUCT_TYPE::UNIFORMS:
				buffer_handling_code += ("const " + oclr_struct->name + " user_buffer_element_" +
										 cur_user_buffer_str + " = *user_buffer_" + cur_user_buffer_str + ";\n");
				main_call_parameters += "&user_buffer_element_" + cur_user_buffer_str + ", ";
				break;
			case oclraster_program::STRUCT_TYPE::IMAGES:
			case oclraster_program::STRUCT_TYPE::FRAMEBUFFER: oclr_unreachable();
		}
		cur_user_buffer++;
	}
	for(const auto& img : images.image_names) {
		main_call_parameters += img + ", ";
	}
	main_call_parameters += "vertex_id, camera_position"; // the same for all transform programs
	core::find_and_replace(program_code, "//###OCLRASTER_USER_PRE_MAIN_CALL###", buffer_handling_code);
	core::find_and_replace(program_code, "//###OCLRASTER_USER_MAIN_CALL###",
						   "oclraster_user_"+entry_function+"("+main_call_parameters+");");
	core::find_and_replace(program_code, "//###OCLRASTER_USER_OUTPUT_COPY###", output_handling_code);
	
	// replace remaining image placeholders
	for(size_t i = 0, img_count = image_decls.size(); i < img_count; i++) {
		core::find_and_replace(program_code, "//###OCLRASTER_IMAGE_"+size_t2string(i)+"###", image_decls[i]);
	}
	
	// done
	//oclr_msg("generated transform user program: %s", program_code);
	return program_code;
}

string transform_program::get_fixed_entry_function_parameters() const {
	return "const int vertex_index, const float3 camera_position";
}

string transform_program::get_qualifier_for_struct_type(const STRUCT_TYPE& type) const {
	switch(type) {
		case STRUCT_TYPE::INPUT:
		case STRUCT_TYPE::UNIFORMS:
			return "const";
		case STRUCT_TYPE::OUTPUT:
			// private memory
			return "";
		case oclraster_program::STRUCT_TYPE::IMAGES:
		case oclraster_program::STRUCT_TYPE::FRAMEBUFFER:
			return "";
	}
}
