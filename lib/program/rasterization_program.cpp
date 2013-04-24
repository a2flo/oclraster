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
)OCLRASTER_RAWSTR"};
#endif

rasterization_program::rasterization_program(const string& code,
											 const string entry_function_,
											 const string build_options_,
											 const kernel_spec default_spec_) :
oclraster_program(code, entry_function_, "-DOCLRASTER_RASTERIZATION_PROGRAM "+build_options_, default_spec_) {
	kernel_function_name = "oclraster_rasterization";
	process_program(code, default_spec_);
}

rasterization_program::~rasterization_program() {
}

string rasterization_program::specialized_processing(const string& code,
													 const kernel_spec& spec) {
	// insert (processed) user code into template program
	string program_code = template_rasterization_program;
	core::find_and_replace(program_code, "//###OCLRASTER_USER_CODE###", code);
	
	//
	vector<string> image_decls;
	const string kernel_parameters { create_user_kernel_parameters(spec, image_decls, true) };
	core::find_and_replace(program_code, "//###OCLRASTER_USER_STRUCTS###", kernel_parameters);
	
	// insert main call + prior buffer handling
	string buffer_handling_code = "";
	string main_call_parameters = "";
	size_t cur_user_buffer = 0;
	bool has_output_structs = false;
	for(const auto& oclr_struct : structs) {
		const string cur_user_buffer_str = size_t2string(cur_user_buffer);
		switch (oclr_struct->type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				// there are no input structs
				continue;
			case oclraster_program::STRUCT_TYPE::OUTPUT: {
				has_output_structs = true;
				const string interp_var_name = "interpolated_user_buffer_element_" + cur_user_buffer_str;
				buffer_handling_code += oclr_struct->name + " " + interp_var_name +";\n";
				for(const auto& var : oclr_struct->variables) {
					buffer_handling_code += interp_var_name + "." + var + " = interpolate(";
					for(size_t i = 0; i < 3; i++) {
						buffer_handling_code += "user_buffer_" + cur_user_buffer_str + "[indices[" + size_t2string(i) + "]]." + var;
						if(i < 2) buffer_handling_code += ", ";
					}
					buffer_handling_code += ", barycentric.xyz);\n";
				}
				main_call_parameters += "&" + interp_var_name + ", ";
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
	if(has_output_structs) {
		// reading indices is only necessary when transform stage output variables must be interpolated
		buffer_handling_code = ("const unsigned int instance_index_offset = instance_id * instance_index_count;\nMAKE_PRIMITIVE_INDICES(indices);\n" +
								buffer_handling_code);
	}
	for(size_t i = 0, img_count = image_decls.size(); i < img_count; i++) {
		// framebuffer is passed in separately
		if(images.is_framebuffer[i]) continue;
		main_call_parameters += images.image_names[i] + ", ";
	}
	main_call_parameters += "&framebuffer, fragment_coord, barycentric.xyz, primitive_id, instance_id"; // the same for all rasterization programs
	core::find_and_replace(program_code, "//###OCLRASTER_USER_MAIN_CALL###",
						   buffer_handling_code+"oclraster_user_"+entry_function+"("+main_call_parameters+");");
	
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
			const auto data_type = spec.image_spec[i].data_type;
			const auto channel_type = spec.image_spec[i].channel_type;
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
	return "oclraster_framebuffer* framebuffer, const float2 fragment_coord, const float3 barycentric, const unsigned int primitive_index, const unsigned int instance_index";
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
