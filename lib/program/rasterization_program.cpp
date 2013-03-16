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

#include "rasterization_program.h"

#if defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
string template_rasterization_program { "" };
#else
static constexpr char template_rasterization_program[] { u8R"OCLRASTER_RAWSTR(
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
		// unused: 13 - 15
		float data[16];
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
	}
)OCLRASTER_RAWSTR"};
#endif

rasterization_program::rasterization_program(const string& code, const string entry_function_) :
oclraster_program(code, entry_function_) {
	process_program(code);
}

rasterization_program::~rasterization_program() {
}

string rasterization_program::specialized_processing(const string& code,
													 const kernel_image_spec& image_spec) {
	// insert (processed) user code into template program
	string program_code = template_rasterization_program;
	core::find_and_replace(program_code, "//###OCLRASTER_USER_CODE###", code);
	
	//
	vector<string> image_decls;
	const string kernel_parameters { create_user_kernel_parameters(image_spec, image_decls) };
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
					buffer_handling_code += "user_buffer_" + cur_user_buffer_str + "[(triangle_id * 3) + " + size_t2string(i) + "]";
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
			case oclraster_program::STRUCT_TYPE::IMAGES:
			case oclraster_program::STRUCT_TYPE::FRAMEBUFFER: oclr_unreachable();
		}
		cur_user_buffer++;
	}
	for(size_t i = 0, img_count = image_decls.size(); i < img_count; i++) {
		// framebuffer is passed in separately
		if(images.is_framebuffer[i]) continue;
		main_call_parameters += images.image_names[i] + ", ";
	}
	main_call_parameters += "&framebuffer, fragment_coord, barycentric.xyz"; // the same for all rasterization programs
	core::find_and_replace(program_code, "//###OCLRASTER_USER_MAIN_CALL###",
						   buffer_handling_code+"_oclraster_user_"+entry_function+"("+main_call_parameters+");");
	
	// image and framebuffer handling
	string framebuffer_read_code = "", framebuffer_write_code = "";
	framebuffer_read_code += "oclraster_framebuffer framebuffer;\n";
	framebuffer_read_code += "const unsigned int framebuffer_offset = (y * framebuffer_size.x) + x;\n";
	for(size_t i = 0, fb_img_idx = 0, img_count = image_decls.size(); i < img_count; i++) {
		if(images.is_framebuffer[i]) {
			// framebuffer type handling
			// -> 8-bit and 16-bit integer and half float formats have to be treated as floats inside the kernel
			// -> do the appropriate input/output data conversion
			// NOTE: 32-bit and 64-bit types (both integer and float) will not be converted to float, since
			// there is no correct conversion for these types and it probably is not wanted in the first place
			const auto data_type = get_image_data_type(image_spec[i]);
			const auto channel_type = get_image_channel_type(image_spec[i]);
			const string native_data_type_str = image_data_type_to_string(data_type);
			const string native_channel_type_str = image_channel_type_to_string(channel_type);
			string native_type = native_data_type_str + native_channel_type_str;
			string type_in_kernel = native_type;
			string input_convert = "";
			string output_convert = "";
			string input_normalization = "))";
			string output_normalization = "))";
			switch(data_type) {
				case IMAGE_TYPE::UINT_8:
				case IMAGE_TYPE::UINT_16:
				case IMAGE_TYPE::INT_8:
				case IMAGE_TYPE::INT_16:
				case IMAGE_TYPE::FLOAT_16:
					type_in_kernel = "float" + native_channel_type_str;
					input_convert = "convert_"+type_in_kernel;
					output_convert = "convert_" + native_type;
					if(data_type != IMAGE_TYPE::FLOAT_16) output_convert += "_sat"; // only allowed for integer formats
					else native_type = "half";
					break;
				default: break;
			}
			switch(data_type) {
				case IMAGE_TYPE::UINT_8:
					input_normalization = ") / 255.0f)";
					output_normalization = ") * 255.0f)";
					break;
				case IMAGE_TYPE::UINT_16:
					input_normalization = ") / 65535.0f)";
					output_normalization = ") * 65535.0f)";
					break;
				case IMAGE_TYPE::INT_8:
					input_normalization = " + 128.0f) / 255.0f) * 2.0f - 1.0f";
					output_normalization = " + 1.0f) * 0.5f) * 255.0f - 128.0f";
					break;
				case IMAGE_TYPE::INT_16:
					input_normalization = " + 32768.0f) / 65535.0f) * 2.0f - 1.0f";
					output_normalization = " + 1.0f) * 0.5f) * 65535.0f - 32768.0f";
					break;
				default: break;
			}
			
			// now that we know the framebuffer type inside the kernel, replace/insert the type in the framebuffer struct declaration
			core::find_and_replace(program_code, "###OCLRASTER_FRAMEBUFFER_IMAGE_"+size_t2string(fb_img_idx)+"###", type_in_kernel);
			
			// framebuffer read/write code
			const string fb_data_ptr_name = "oclr_framebuffer_ptr_"+images.image_names[i];
			const string const_str = (images.image_specifiers[i] == ACCESS_TYPE::READ &&
									  images.image_types[i] == IMAGE_VAR_TYPE::IMAGE_2D ?
									  " const" : "");
			framebuffer_read_code += ("global"+const_str+" "+native_type+"* "+fb_data_ptr_name+
									  " = (global"+const_str+" "+native_type+
									  "*)((global"+const_str+" uchar*)oclr_framebuffer_"+images.image_names[i]+
									  " + OCLRASTER_IMAGE_HEADER_SIZE);\n");
			
			framebuffer_read_code += "framebuffer."+images.image_names[i]+" = ";
			if(data_type != IMAGE_TYPE::FLOAT_16) {
				framebuffer_read_code += "(("+input_convert+"("+fb_data_ptr_name+"[framebuffer_offset])"+input_normalization+";\n";
				framebuffer_write_code += fb_data_ptr_name+"[framebuffer_offset] = ";
				framebuffer_write_code += output_convert+"(((framebuffer."+images.image_names[i]+output_normalization+");\n";
			}
			else {
				// look! it's a three-headed monkey!
				framebuffer_read_code += "vload_half"+native_channel_type_str+"(framebuffer_offset, "+fb_data_ptr_name+");\n";
				framebuffer_write_code += "vstore_half"+native_channel_type_str+"(framebuffer."+images.image_names[i]+", ";
				framebuffer_write_code += "framebuffer_offset, (global half*)"+fb_data_ptr_name+");\n";
			}
			if(images.image_types[i] == IMAGE_VAR_TYPE::DEPTH_IMAGE) {
				framebuffer_read_code += "float* fragment_depth = &framebuffer."+images.image_names[i]+";\n";
				framebuffer_read_code += "const float input_depth = *fragment_depth;\n";
			}
			
			fb_img_idx++;
		}
		core::find_and_replace(program_code, "###OCLRASTER_IMAGE_"+size_t2string(i)+"###", image_decls[i]);
	}
	core::find_and_replace(program_code, "//###OCLRASTER_FRAMEBUFFER_READ###", framebuffer_read_code);
	core::find_and_replace(program_code, "//###OCLRASTER_FRAMEBUFFER_WRITE###", framebuffer_write_code);
	
	// done
	//oclr_msg("generated rasterize user program: %s", program_code);
	return program_code;
}

string rasterization_program::get_fixed_entry_function_parameters() const {
	return "oclraster_framebuffer* framebuffer, const float2 fragment_coord, const float3 barycentric";
}

string rasterization_program::get_qualifier_for_struct_type(const STRUCT_TYPE& type) const {
	switch(type) {
		case STRUCT_TYPE::INPUT:
			// there are no input structs
			return "";
		case STRUCT_TYPE::UNIFORMS:
		case STRUCT_TYPE::OUTPUT:
			return "const";
		case oclraster_program::STRUCT_TYPE::IMAGES:
		case oclraster_program::STRUCT_TYPE::FRAMEBUFFER:
			return "";
	}
}
