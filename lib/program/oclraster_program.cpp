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

#include "oclraster_program.h"
#include "oclraster.h"
#include <regex>

#include "libtcc.h"
extern "C" {
#include "tcc.h"
}

oclraster_program::oclraster_program(const string& code oclr_unused,
									 const string entry_function_,
									 const string build_options_,
									 const kernel_spec default_spec_ oclr_unused) :
entry_function(entry_function_), build_options(build_options_), kernel_function_name("oclraster_program") {
}

oclraster_program::~oclraster_program() {
	for(auto& spec : compiled_kernels) {
		delete spec;
	}
	for(auto& oclr_struct : structs) {
		delete oclr_struct;
	}
	if(ocl != nullptr) {
		for(const auto& kernel : kernels) {
			ocl->delete_kernel(kernel.second);
		}
	}
}

void oclraster_program::process_program(const string& raw_code, const kernel_spec default_spec) {
	// preprocess
	const string code = preprocess_code(raw_code);
	
	// parse
	static const array<const pair<const char*, const STRUCT_TYPE>, 6> oclraster_struct_types {
		{
			{ u8"oclraster_in", STRUCT_TYPE::INPUT },
			{ u8"oclraster_out", STRUCT_TYPE::OUTPUT },
			{ u8"oclraster_uniforms", STRUCT_TYPE::UNIFORMS },
			{ u8"oclraster_buffers", STRUCT_TYPE::BUFFERS },
			{ u8"oclraster_images", STRUCT_TYPE::IMAGES },
			{ u8"oclraster_framebuffer", STRUCT_TYPE::FRAMEBUFFER }
		}
	};
	static const set<const string> specifiers {
		"read_only", "write_only", "read_write"
	};
	
	// current oclraster_struct grammar limitations/requirements:
	// * no interior/nested structs/unions
	// * no multi-variable declarations (e.g. "float x, y, z;")
	// * no __attribute__ (oclraster_structs already have a __attribute__ qualifier)
	// * use of any oclraster_struct specifier in other places is disallowed (no typedefs, comments, ...)
	// * otherwise standard OpenCL C
	//
	// example:
	// oclraster_in vertex_input {
	// 		float4 vertex;
	// 		float4 normal;
	// 		float2 tex_coord;
	// } inputs;
	//
	vector<size2> image_struct_positions;
	try {
		// parse and extract
		for(const auto& type : oclraster_struct_types) {
			size_t struct_pos = 0;
			while((struct_pos = code.find(type.first, struct_pos)) != string::npos) {
				// find first ' ' space char and open '{' bracket and extract the structs name
				const size_t space_pos = code.find_first_of(" {", struct_pos);
				const size_t open_bracket_pos = code.find("{", struct_pos);
				if(space_pos == string::npos || code[space_pos] == '{') throw oclraster_exception("no struct name");
				if(open_bracket_pos == string::npos) throw oclraster_exception("no struct open bracket");
				const string struct_name = core::trim(code.substr(space_pos+1, open_bracket_pos-space_pos-1));
				//oclr_msg("struct type: \"%s\"", type.first);
				//oclr_msg("struct name: \"%s\"", struct_name);
				
				// open/close bracket match
				size_t bracket_pos = open_bracket_pos;
				size_t open_bracket_count = 1;
				while(open_bracket_count > 0) {
					bracket_pos = code.find_first_of("{}", bracket_pos + 1);
					if(bracket_pos == string::npos) throw oclraster_exception("struct open/close bracket mismatch");
					code[bracket_pos] == '{' ? open_bracket_count++ : open_bracket_count--;
				}
				const size_t close_bracket_pos = bracket_pos;
				
				//
				const size_t end_semicolon_pos = code.find(";", close_bracket_pos+1);
				if(end_semicolon_pos == string::npos) {
					throw oclraster_exception("end-semicolon missing from struct \""+struct_name+"\"!");
				}
				const string object_name = core::trim(code.substr(close_bracket_pos+1,
																  end_semicolon_pos-close_bracket_pos-1));
				//oclr_msg("object name: \"%s\"", object_name);
				
				//
				string struct_interior = code.substr(open_bracket_pos+1, close_bracket_pos-open_bracket_pos-1);
				//oclr_msg("struct interior:\n\t%s\n", struct_interior);
				
				// strip unnecessary whitespace and comments, and condense
				static const regex rx_space("\\s+");
				static const regex rx_semicolon_space("[ ]*;[ ]*");
				static const regex rx_newline("\n|\r");
				static const regex rx_comments_sl("//(.*)");
				static const regex rx_comments_ml("/\\*(.*)\\*/");

				struct_interior = regex_replace(struct_interior, rx_comments_sl, "");
				struct_interior = regex_replace(struct_interior, rx_newline, "");
				struct_interior = regex_replace(struct_interior, rx_comments_ml, "");
				struct_interior = regex_replace(struct_interior, rx_space, " ");
				struct_interior = regex_replace(struct_interior, rx_semicolon_space, ";");
				struct_interior = core::trim(struct_interior);
				//oclr_msg("post-regex interior: >%s<", struct_interior);
				
				// extract all member variables
				vector<string> variable_names, variable_types, variable_specifiers;
				size_t semicolon_pos = 0, last_semicolon_pos = 0;
				while((semicolon_pos = struct_interior.find(";", last_semicolon_pos)) != string::npos) {
					const string var_decl = struct_interior.substr(last_semicolon_pos,
																   semicolon_pos-last_semicolon_pos);
					//oclr_msg("decl: >%s<", var_decl);
					
					const size_t name_start_pos = var_decl.rfind(" ");
					if(name_start_pos == string::npos) {
						throw oclraster_exception("invalid variable declaration: \""+var_decl+"\"");
					}
					const string var_name = var_decl.substr(name_start_pos+1, var_decl.length()-name_start_pos-1);
					//oclr_msg("name: >%s<", var_name);
					variable_names.emplace_back(var_name);
					
					// check if type has an additional specifier (for images: read_only, write_only, read_write)
					const size_t type_start_pos = var_decl.find(" ");
					const string start_token = var_decl.substr(0, type_start_pos);
					if(specifiers.find(start_token) != specifiers.end()) {
						const string var_type = regex_replace(var_decl.substr(type_start_pos+1, name_start_pos-type_start_pos-1),
															  rx_space, ""); // need to strip any whitespace
						//oclr_msg("type (s): >%s<", var_type);
						variable_types.emplace_back(var_type);
						
						const string var_spec = var_decl.substr(0, type_start_pos);
						//oclr_msg("spec: >%s<", var_spec);
						variable_specifiers.emplace_back(var_spec);
					}
					else {
						const string var_type = core::trim(var_decl.substr(0, name_start_pos));
						//oclr_msg("type: >%s<", var_type);
						variable_types.emplace_back(var_type);
						variable_specifiers.emplace_back("");
					}
					
					// continue
					last_semicolon_pos = semicolon_pos+1;
				}
				
				// create info struct
				if(type.second != STRUCT_TYPE::IMAGES &&
				   type.second != STRUCT_TYPE::FRAMEBUFFER) {
					const bool empty = (variable_names.size() == 0); // can't use variable_names when moving
					structs.push_back(new oclraster_struct_info {
						type.second,
						size2(struct_pos, end_semicolon_pos+1),
						struct_name,
						object_name,
						std::move(variable_names),
						std::move(variable_types),
						std::move(variable_specifiers),
						empty,
						{}
					});
				}
				else {
					image_struct_positions.emplace_back(size2 { struct_pos, end_semicolon_pos+1 });
					process_image_struct(variable_names, variable_types, variable_specifiers,
										 (type.second == STRUCT_TYPE::FRAMEBUFFER));
				}
				
				// continue
				struct_pos++;
			}
		}
		
		// process found structs
		for(auto& oclr_struct : structs) {
			if(oclr_struct->empty) continue;
			if(oclr_struct->type == STRUCT_TYPE::BUFFERS) continue;
			generate_struct_info_cl_program(*oclr_struct);
		}
		
		// order
		sort(structs.begin(), structs.end(),
			 [](const oclraster_struct_info* info_0, const oclraster_struct_info* info_1) -> bool {
				 return info_0->code_pos.x < info_1->code_pos.x;
			 });
		
		// write framebuffer struct
		bool has_framebuffer = false;
		string framebuffer_code = "typedef struct __attribute__((packed)) {\n";
		for(size_t i = 0, fb_img_idx = 0, image_count = images.image_names.size(); i < image_count; i++) {
			if(!images.is_framebuffer[i]) continue;
			has_framebuffer = true;
			// TODO: for now, let the access always be read/write, so "const data" (depth) can be modified
			// between user program calls (downside: user has also read/write access -> create 2 structs?)
			/*if(images.image_specifiers[i] == ACCESS_TYPE::READ) {
				framebuffer_code += "const ";
			}*/
			framebuffer_code += "###OCLRASTER_FRAMEBUFFER_IMAGE_" + size_t2string(fb_img_idx) + "### " + images.image_names[i] + ";\n";
			fb_img_idx++;
		}
		framebuffer_code += "} oclraster_framebuffer;\n";
		
		// recreate structs (in reverse, so that the offsets stay valid)
		processed_code = code;
		const size_t struct_count = structs.size() + image_struct_positions.size();
		for(size_t i = 0, cur_struct = structs.size(), cur_image = image_struct_positions.size();
			i < struct_count; i++) {
			// figure out which struct comes next (normal struct or image struct)
			size_t image_code_pos = 0, struct_code_pos = 0;
			if(cur_image > 0) image_code_pos = image_struct_positions[cur_image-1].x;
			if(cur_struct > 0) struct_code_pos = structs[cur_struct-1]->code_pos.x;
			
			// image
			if(image_code_pos > struct_code_pos) {
				cur_image--;
				processed_code.erase(image_struct_positions[cur_image].x,
									 image_struct_positions[cur_image].y - image_struct_positions[cur_image].x);
				
				// insert framebuffer struct code at the last image or framebuffer struct position
				if(has_framebuffer && cur_image == (image_struct_positions.size()-1)) {
					processed_code.insert(image_struct_positions[cur_image].x, framebuffer_code);
				}
			}
			// struct
			else {
				cur_struct--;
				const oclraster_struct_info& oclr_struct = *structs[cur_struct];
				processed_code.erase(oclr_struct.code_pos.x,
									 oclr_struct.code_pos.y - oclr_struct.code_pos.x);
				
				if(!oclr_struct.empty && oclr_struct.type != STRUCT_TYPE::BUFFERS) {
					string struct_code = "";
					switch(oclr_struct.type) {
						case STRUCT_TYPE::INPUT:
							struct_code += "oclraster_in";
							break;
						case STRUCT_TYPE::OUTPUT:
							struct_code += "oclraster_out";
							break;
						case STRUCT_TYPE::UNIFORMS:
							struct_code += "oclraster_uniforms";
							break;
						case STRUCT_TYPE::BUFFERS:
						case STRUCT_TYPE::IMAGES:
						case STRUCT_TYPE::FRAMEBUFFER: oclr_unreachable();
					}
					struct_code += " {\n";
					for(size_t var_index = 0; var_index < oclr_struct.variables.size(); var_index++) {
						struct_code += oclr_struct.variable_types[var_index] + " " + oclr_struct.variables[var_index] + ";\n";
					}
					struct_code += "} " + oclr_struct.name + ";\n";
					processed_code.insert(oclr_struct.code_pos.x, struct_code);
				}
			}
		}
		
		// remove empty structs
		for(auto iter = structs.begin(); iter != structs.end();) {
			if((*iter)->empty) {
				delete *iter;
				iter = structs.erase(iter);
			}
			else iter++;
		}
		
		// build entry function parameter string
		const string entry_function_params = create_entry_function_parameters();
		
		// check if entry function exists, and if so, replace it with a modified function name
		const regex rx_entry_function("("+entry_function+")\\s*\\(\\s*\\)");
		if(!regex_search(code, rx_entry_function)) {
			throw oclraster_exception("entry function \""+entry_function+"\" not found!");
		}
		processed_code = regex_replace(processed_code, rx_entry_function,
									   "oclraster_user_"+entry_function+"("+entry_function_params+")");
		
		// create default/first/hinted image spec, do the final processing and compile
		kernel_spec spec { default_spec };
		if(!images.image_names.empty() && spec.image_spec.size() != images.image_names.size()) {
			// create kernel image spec for the hinted or default image specs
			// note: if default_spec already contains some image_spec entries, only insert the remaining ones
			for(size_t i = spec.image_spec.size(); i < images.image_names.size(); i++) {
				spec.image_spec.emplace_back(images.image_hints[i].is_valid() ?
											 images.image_hints[i] :
											 image_type { IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA });
			}
		}
		else if(images.image_names.empty() && !spec.image_spec.empty()) {
			// default spec contains image_spec entries, but the are no images -> clear
			spec.image_spec.clear();
		}
		// else: no images in kernel/program -> just one kernel / "empty image spec"
		build_kernel(spec);
	}
	catch(oclraster_exception& ex) {
		invalidate(ex.what());
	}
	valid = true;
}

weak_ptr<opencl::kernel_object> oclraster_program::build_kernel(const kernel_spec& spec) {
	kernel_spec* new_spec = new kernel_spec(spec);
	compiled_kernels.emplace_back(new_spec);
	
	// if any device doesn't support doubles and the user tries to use a double image format,
	// fail immediately and return a null kernel (better here than crashing during compilation)
	if(!ocl->is_full_double_support()) {
		for(const auto& img_type : spec.image_spec) {
			if(img_type.data_type == IMAGE_TYPE::FLOAT_64) {
				oclr_error("can't use a double/FLOAT_64 image format when one or more opencl devices do not support doubles!");
				kernels.emplace(new_spec, opencl::null_kernel_object);
				return opencl::null_kernel_object;
			}
		}
	}
	
	// build image defines string (image functions for each image type are #ifdef'ed)
	string image_defines = "";
	set<string> img_types;
	for(const auto& img_type : spec.image_spec) {
		if(img_type.native) continue;
		img_types.insert(img_type.to_string());
	}
	for(const auto& img_type : img_types) {
		image_defines += " -DOCLRASTER_IMAGE_" + core::str_to_upper(img_type);
	}
	
	// depth defines
	string framebuffer_options = "";
	bool has_framebuffer_depth = false;
	for(size_t i = 0, img_count = images.image_names.size(); i < img_count; i++) {
		if(images.is_framebuffer[i] && images.image_types[i] == IMAGE_VAR_TYPE::DEPTH_IMAGE) {
			has_framebuffer_depth = true;
			break;
		}
	}
	if(!has_framebuffer_depth) framebuffer_options += " -DOCLRASTER_NO_DEPTH";
	if(!spec.depth.depth_test) framebuffer_options += " -DOCLRASTER_NO_DEPTH_TEST";
	if(spec.depth.depth_override) framebuffer_options += " -DOCLRASTER_DEPTH_OVERRIDE";
	
	string depth_spec_str = "";
	depth_spec_str += (spec.depth.depth_test ? ".depth_test" : ".no_depth_test");
	depth_spec_str += ".";
	switch(spec.depth.depth_func) {
		case DEPTH_FUNCTION::NEVER: depth_spec_str += "never"; break;
		case DEPTH_FUNCTION::LESS: depth_spec_str += "less"; break;
		case DEPTH_FUNCTION::EQUAL: depth_spec_str += "equal"; break;
		case DEPTH_FUNCTION::LESS_OR_EQUAL: depth_spec_str += "lequal"; break;
		case DEPTH_FUNCTION::GREATER: depth_spec_str += "greater"; break;
		case DEPTH_FUNCTION::NOT_EQUAL: depth_spec_str += "nequal"; break;
		case DEPTH_FUNCTION::GREATER_OR_EQUAL: depth_spec_str += "gequal"; break;
		case DEPTH_FUNCTION::ALWAYS: depth_spec_str += "always"; break;
		case DEPTH_FUNCTION::CUSTOM: depth_spec_str += "custom"; break;
	}
	depth_spec_str += (spec.depth.depth_override ? ".depth_override" : "");
	
	// finally: call the specialized processing function of inheriting classes/programs
	// note: this should inject the user code into their respective code templates
	const string program_code { specialized_processing(processed_code, *new_spec) };
	
	//
	const string proj_spec_str = (spec.projection == PROJECTION::PERSPECTIVE ? "perspective" : "orthographic");
	string img_spec_str = "";
	for(const auto& type : spec.image_spec) {
		img_spec_str += "." + type.to_string();
	}
	
	stringstream id_stream;
	id_stream << dec << this_thread::get_id();
	const string identifier = ("USER_PROGRAM."+kernel_function_name+"."+entry_function+"."+
							   proj_spec_str+depth_spec_str+img_spec_str+"."+
							   ull2string(SDL_GetPerformanceCounter())+"."+id_stream.str());
	weak_ptr<opencl::kernel_object> kernel = ocl->add_kernel_src(identifier, program_code, kernel_function_name,
																 " -DBIN_SIZE="+uint2string(OCLRASTER_BIN_SIZE)+
																 " -DBATCH_SIZE="+uint2string(OCLRASTER_BATCH_SIZE)+
																 " -DOCLRASTER_PROJECTION_"+(spec.projection == PROJECTION::PERSPECTIVE ? "PERSPECTIVE" : "ORTHOGRAPHIC")+
																 image_defines+
																 framebuffer_options+
																 " "+build_options);
	//oclr_msg("%s:\n%s\n", identifier, program_code);
#if defined(OCLRASTER_DEBUG)
	if(kernel.use_count() == 0) {
		oclr_debug("kernel source: %s", program_code);
	}
#endif
	kernels.emplace(new_spec, kernel);
	return kernel;
}

string oclraster_program::create_entry_function_parameters() const {
	const string fixed_params = get_fixed_entry_function_parameters();
	string entry_function_params = "";
	for(size_t i = 0, struct_count = structs.size(); i < struct_count; i++) {
		if(structs[i]->type != STRUCT_TYPE::BUFFERS) {
			const string qualifier = get_qualifier_for_struct_type(structs[i]->type);
			if(qualifier != "") entry_function_params += qualifier + " ";
			entry_function_params += structs[i]->name + "* " + structs[i]->object_name + ", ";
		}
		else {
			for(size_t j = 0, buffer_entries = structs[i]->variables.size(); j < buffer_entries; j++) {
				entry_function_params += "global " + structs[i]->variable_types[j] + " " + structs[i]->variables[j] + ", ";
			}
		}
	}
	for(size_t i = 0, image_count = images.image_names.size(); i < image_count; i++) {
		// framebuffer is passed in separately
		if(images.is_framebuffer[i]) continue;
		
		// for images: only add a placeholder (will be replaced by the actual image type later on)
		entry_function_params += "###OCLRASTER_IMAGE_"+size_t2string(i)+"###, ";
	}
	entry_function_params += fixed_params;
	return entry_function_params;
}

string oclraster_program::create_user_kernel_parameters(const kernel_spec& spec,
														vector<string>& image_decls,
														const bool const_output) const {
	// creates user buffer dependent kernel parameters string (buffers will be called "user_buffer_#")
	string kernel_parameters = "";
	size_t user_buffer_count = 0;
	for(const auto& oclr_struct : structs) {
		//
		if(oclr_struct->type == oclraster_program::STRUCT_TYPE::BUFFERS) {
			for(size_t i = 0, buffer_entries = oclr_struct->variables.size(); i < buffer_entries; i++) {
				kernel_parameters += "global " + oclr_struct->variable_types[i] + " user_buffer_"+size_t2string(user_buffer_count)+",\n";
				user_buffer_count++;
			}
			continue;
		}
		
		//
		switch(oclr_struct->type) {
			case oclraster_program::STRUCT_TYPE::INPUT:
				kernel_parameters += "global const ";
				break;
			case oclraster_program::STRUCT_TYPE::OUTPUT:
				kernel_parameters += "global ";
				if(const_output) kernel_parameters += "const ";
				break;
			case oclraster_program::STRUCT_TYPE::UNIFORMS:
				kernel_parameters += "constant ";
				break;
			case oclraster_program::STRUCT_TYPE::BUFFERS:
			case oclraster_program::STRUCT_TYPE::IMAGES:
			case oclraster_program::STRUCT_TYPE::FRAMEBUFFER: oclr_unreachable();
		}
		kernel_parameters += oclr_struct->name + "* user_buffer_"+size_t2string(user_buffer_count)+",\n";
		user_buffer_count++;
	}
	for(size_t i = 0, img_count = images.image_names.size(); i < img_count; i++) {
		string type_str = "";
		if(!spec.image_spec[i].native) {
			// buffer based image
			type_str = "global ";
			if(images.image_specifiers[i] == ACCESS_TYPE::READ &&
			   !images.is_framebuffer[i]) {
				type_str += "const ";
			}
			if(spec.image_spec[i].data_type == IMAGE_TYPE::FLOAT_16) {
				// if cl_khr_fp16 is not supported (-> all implementations ...), half vector types are not supported
				// and structs containing halfs (the workaround) are not allowed as kernel function parameter types
				// -> use custom half pointer type for distinction and later type-casting (note: type is correctly aligned)
				type_str += "oclr_";
			}
			type_str += spec.image_spec[i].to_string();
			type_str += "* ";
			if(images.is_framebuffer[i]) type_str += "oclr_framebuffer_";
		}
		else {
			// native image
			if(images.image_specifiers[i] == ACCESS_TYPE::READ) {
				type_str += "read_only ";
			}
			else if(images.image_specifiers[i] == ACCESS_TYPE::WRITE) {
				type_str += "write_only ";
			}
			else {
				// this shouldn't actually happen and be caught much earlier, but you never know ...
				oclr_error("native images can not have a read_write access qualifier!");
				type_str += "read_only "; // default to read_only
			}
			type_str += "image2d_t ";
		}
		type_str += images.image_names[i];
		kernel_parameters += type_str+",\n";
		image_decls.emplace_back(type_str);
	}
	return kernel_parameters;
}

void oclraster_program::process_image_struct(const vector<string>& variable_names,
											 const vector<string>& variable_types,
											 const vector<string>& variable_specifiers,
											 const bool is_framebuffer) {
	//
	vector<IMAGE_VAR_TYPE> image_types;
	vector<ACCESS_TYPE> image_specifiers;
	vector<image_type> image_hints;
	
	//
	for(size_t i = 0, image_count = variable_types.size(); i < image_count; i++) {
		const auto& var_type = variable_types[i];
		const auto& var_spec = variable_specifiers[i];
		
		const auto hint_start = var_type.find("<");
		const auto hint_end = var_type.find(">", hint_start);
		const bool has_hint = (hint_start != string::npos && hint_end != string::npos);
		if((hint_start != string::npos && hint_end == string::npos) ||
		   (hint_start == string::npos && hint_end != string::npos)) {
			throw oclraster_exception("invalid image declaration: \""+var_type+"\"");
		}
		
		// figure out the image var type of this image
		const string image_type = (has_hint ? var_type.substr(0, hint_start) : var_type);
		IMAGE_VAR_TYPE image_var_type = IMAGE_VAR_TYPE::IMAGE_2D;
		if(!is_framebuffer) {
			// image
			if(image_type == "image1d") image_var_type = IMAGE_VAR_TYPE::IMAGE_1D;
			else if(image_type == "image2d") image_var_type = IMAGE_VAR_TYPE::IMAGE_2D;
			else if(image_type == "image3d") image_var_type = IMAGE_VAR_TYPE::IMAGE_3D;
			else throw oclraster_exception("invalid image declaration: \""+var_type+"\"");
		}
		else {
			// framebuffer
			if(image_type == "image2d") image_var_type = IMAGE_VAR_TYPE::IMAGE_2D;
			else if(image_type == "depth_image") image_var_type = IMAGE_VAR_TYPE::DEPTH_IMAGE;
			else if(image_type == "stencil_image") image_var_type = IMAGE_VAR_TYPE::STENCIL_IMAGE;
			else throw oclraster_exception("invalid framebuffer image declaration: \""+var_type+"\"");
		}
		image_types.emplace_back(image_var_type);
		
		//
		IMAGE_TYPE img_data_type = IMAGE_TYPE::NONE;
		IMAGE_CHANNEL img_channel_type = IMAGE_CHANNEL::NONE;
		if(has_hint) {
			const string hint = var_type.substr(hint_start+1, hint_end-hint_start-1);
			const auto comma_pos = hint.find(",");
			if(comma_pos == string::npos) throw oclraster_exception("invalid image hint declaration: \""+hint+"\"");
			
			const string data_type = hint.substr(0, comma_pos);
			const string channel_type = hint.substr(comma_pos+1, hint.length()-comma_pos-1);
			
			if(data_type == "INT_8") img_data_type = IMAGE_TYPE::INT_8;
			else if(data_type == "INT_16") img_data_type = IMAGE_TYPE::INT_16;
			else if(data_type == "INT_32") img_data_type = IMAGE_TYPE::INT_32;
			else if(data_type == "INT_64") img_data_type = IMAGE_TYPE::INT_64;
			else if(data_type == "UINT_8") img_data_type = IMAGE_TYPE::UINT_8;
			else if(data_type == "UINT_16") img_data_type = IMAGE_TYPE::UINT_16;
			else if(data_type == "UINT_32") img_data_type = IMAGE_TYPE::UINT_32;
			else if(data_type == "UINT_64") img_data_type = IMAGE_TYPE::UINT_64;
			else if(data_type == "FLOAT_16") img_data_type = IMAGE_TYPE::FLOAT_16;
			else if(data_type == "FLOAT_32") img_data_type = IMAGE_TYPE::FLOAT_32;
			else if(data_type == "FLOAT_64") img_data_type = IMAGE_TYPE::FLOAT_64;
			else throw oclraster_exception("invalid image hint declaration (invalid data type): \""+hint+"\"");
			
			if(channel_type == "R") img_channel_type = IMAGE_CHANNEL::R;
			else if(channel_type == "RG") img_channel_type = IMAGE_CHANNEL::RG;
			else if(channel_type == "RGB") img_channel_type = IMAGE_CHANNEL::RGB;
			else if(channel_type == "RGBA") img_channel_type = IMAGE_CHANNEL::RGBA;
			else throw oclraster_exception("invalid image hint declaration (invalid channel type): \""+hint+"\"");
			
			image_hints.emplace_back(img_data_type, img_channel_type);
		}
		else {
			if(image_var_type == IMAGE_VAR_TYPE::DEPTH_IMAGE) {
				image_hints.emplace_back(IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R);
			}
			else if(image_var_type == IMAGE_VAR_TYPE::STENCIL_IMAGE) {
				image_hints.emplace_back(IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::R);
			}
			else image_hints.emplace_back(); // will default to UINT_8/RGBA later on
		}
		
		// check hints of framebuffer image types
		if(has_hint) {
			if(image_var_type == IMAGE_VAR_TYPE::DEPTH_IMAGE) {
				throw oclraster_exception("depth_image hint is not allowed!");
			}
			else if(image_var_type == IMAGE_VAR_TYPE::STENCIL_IMAGE) {
				if(img_data_type != IMAGE_TYPE::UINT_8 &&
				   img_data_type != IMAGE_TYPE::UINT_16 &&
				   img_data_type != IMAGE_TYPE::UINT_32 &&
				   img_data_type != IMAGE_TYPE::UINT_64) {
					throw oclraster_exception("stencil_image hint: data type must be UINT_* (not " + image_hints.back().to_string() + ")!");
				}
				else if(img_channel_type != IMAGE_CHANNEL::R) {
					throw oclraster_exception("stencil_image hint: channel type must be R (not " + image_hints.back().to_string() + ")!");
				}
			}
		}
		
		// specifier handling
		if(!is_framebuffer) {
			// image (-> empty specifier is implicitly read/write)
			if(var_spec == "read_only") image_specifiers.emplace_back(ACCESS_TYPE::READ);
			else if(var_spec == "write_only") image_specifiers.emplace_back(ACCESS_TYPE::WRITE);
			else if(var_spec == "read_write" || var_spec == "") {
				image_specifiers.emplace_back(ACCESS_TYPE::READ_WRITE);
			}
			else throw oclraster_exception("invalid image access specifier: \""+var_spec+"\"");
		}
		else {
			// framebuffer
			// -> empty image specifier is implicitly read/write
			// -> empty depth image specifier is implicitly read only
			// -> empty stencil image specifier is implicitly read only
			if(var_spec == "read_only") image_specifiers.emplace_back(ACCESS_TYPE::READ);
			else if(var_spec == "write_only") image_specifiers.emplace_back(ACCESS_TYPE::WRITE);
			else if(var_spec == "read_write") image_specifiers.emplace_back(ACCESS_TYPE::READ_WRITE);
			else if(var_spec == "") {
				image_specifiers.emplace_back((image_var_type == IMAGE_VAR_TYPE::DEPTH_IMAGE ||
											   image_var_type == IMAGE_VAR_TYPE::STENCIL_IMAGE) ?
											  ACCESS_TYPE::READ : ACCESS_TYPE::READ_WRITE);
			}
			else throw oclraster_exception("invalid framebuffer image access specifier: \""+var_spec+"\"");
		}
	}
	
	// append to kernel image container (there is no need to store each image struct separately,
	// since images are treated specially in their entirety, and they are used differently by the user,
	// i.e. each image in itself is set/bound separately and not in complete multi-image-blocks)
	images.image_names.insert(images.image_names.end(), variable_names.begin(), variable_names.end());
	images.image_types.insert(images.image_types.end(), image_types.begin(), image_types.end());
	images.image_specifiers.insert(images.image_specifiers.end(), image_specifiers.begin(), image_specifiers.end());
	images.image_hints.insert(images.image_hints.end(), image_hints.begin(), image_hints.end());
	images.is_framebuffer.insert(images.is_framebuffer.end(), variable_names.size(), is_framebuffer);
}

void oclraster_program::generate_struct_info_cl_program(oclraster_struct_info& struct_info) {
	static const string kernel_header = "#include \"oclr_global.h\"\n#include \"oclr_matrix.h\"\n";
	static const string kernel_start = "kernel void struct_info(global int* info_buffer) {\nint index = 0;\n";
	static const string kernel_end = "}";
	
	string kernel_code = kernel_header;
	
	// struct decl
	kernel_code += "oclraster_struct {\n";
	for(size_t i = 0; i < struct_info.variables.size(); i++) {
		kernel_code += struct_info.variable_types[i] + " " + struct_info.variables[i] + ";\n";
	}
	kernel_code += "} " + struct_info.name + ";\n";
	
	// actual kernel
	kernel_code += kernel_start;
	kernel_code += "atomic_xchg(&info_buffer[index++], (int)sizeof("+struct_info.name+"));\n";
	for(const auto& var : struct_info.variables) {
		// standard c ftw
		kernel_code += "atomic_xchg(&info_buffer[index++], (int)sizeof(("+struct_info.name+"*)0)->"+var+");\n"; // size
		kernel_code += "atomic_xchg(&info_buffer[index++], (int)&((("+struct_info.name+"*)0)->"+var+"));\n"; // offset
	}
	kernel_code += "}"; // eol
	
	//oclr_debug("generated kernel file:\n%s\n", kernel_code);
	
	// and compile
	stringstream id_stream;
	id_stream << dec << this_thread::get_id();
	const string unique_identifier = "STRUCT_INFO."+ull2string(SDL_GetPerformanceCounter())+"."+id_stream.str();
	weak_ptr<opencl::kernel_object> kernel_obj = ocl->add_kernel_src(unique_identifier, kernel_code, "struct_info");
	auto kernel_ptr = kernel_obj.lock();
	if(kernel_ptr == nullptr) {
		oclr_error("failed to create STRUCT_INFO kernel!");
		return;
	}
	
	const size_t info_buffer_size = 1 + struct_info.variables.size() * 2;
	int* info_buffer_results = new int[info_buffer_size];
	
	ocl->lock();
	auto active_device = ocl->get_active_device();
	const auto& devices = ocl->get_devices();
	for(size_t dev_num = 0; dev_num < devices.size(); dev_num++) {
		// this has to be executed for all devices, since each device can have its own struct/member sizes/offsets
		ocl->set_active_device(devices[dev_num]->type);
		//oclr_msg("DEVICE: %s", devices[dev_num]->name);
		
		// read/write is necessary, because of atomic_xchg
		opencl::buffer_object* info_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
																opencl::BUFFER_FLAG::BLOCK_ON_READ,
																info_buffer_size * sizeof(int));
		
		ocl->use_kernel(unique_identifier);
		ocl->set_kernel_argument(0, info_buffer);
		ocl->set_kernel_range({1, 1});
		ocl->run_kernel();
		
		ocl->read_buffer(info_buffer_results, info_buffer);
		
		oclraster_struct_info::device_struct_info dev_info;
		dev_info.struct_size = info_buffer_results[0];
		//oclr_msg("struct \"%s\" size: %d", struct_info.name, dev_info.struct_size);
		dev_info.sizes.resize(struct_info.variables.size());
		dev_info.offsets.resize(struct_info.variables.size());
		for(size_t i = 0; i < struct_info.variables.size(); i++) {
			dev_info.sizes[i] = info_buffer_results[(i*2) + 1];
			dev_info.offsets[i] = info_buffer_results[(i*2) + 2];
			//oclr_msg("\tmember \"%s\": size: %d, offset: %d",
			//		 struct_info.variables[i], dev_info.sizes[i], dev_info.offsets[i]);
		}
		struct_info.device_infos.emplace(devices[dev_num], dev_info);
		
		ocl->delete_buffer(info_buffer);
	}
	ocl->set_active_device(active_device->type);
	ocl->unlock();
	
	// cleanup
	delete [] info_buffer_results;
	kernel_ptr = nullptr;
	ocl->delete_kernel(kernel_obj);
}

bool oclraster_program::is_valid() const {
	return valid;
}

void oclraster_program::invalidate(const string error_info) {
	valid = false;
	oclr_error("there was an error processing your program%s",
			   error_info != "" ? ": " + error_info + "!" : "");
}

const vector<oclraster_program::oclraster_struct_info*>& oclraster_program::get_structs() const {
	return structs;
}

const oclraster_program::oclraster_image_info& oclraster_program::get_images() const {
	return images;
}

weak_ptr<opencl::kernel_object> oclraster_program::get_kernel(const kernel_spec spec) {
	//
	if(kernels.empty() || compiled_kernels.empty()) {
		oclr_error("no kernel has been compiled for this program!");
		return opencl::null_kernel_object;
	}
	if(spec.image_spec.size() != compiled_kernels[0]->image_spec.size()) {
		oclr_error("invalid kernel image spec size (%u) - should be (%u)!",
				   spec.image_spec.size(), compiled_kernels[0]->image_spec.size());
		return opencl::null_kernel_object;
	}
	
	//
	for(const auto& kernel : kernels) {
		if(*kernel.first != spec) continue;
		return kernel.second;
	}
	// new kernel image spec -> compile new kernel
	return build_kernel(spec);
}

string oclraster_program::preprocess_code(const string& raw_code) {
	// init
	string ret_code = "";
	TCCState* state = tcc_new();
	state->output_type = TCC_OUTPUT_PREPROCESS;
	
	// split build options and let tcc parse them
	const string kernels_include_path = "-I" + core::strip_path(oclraster::kernel_path("")) + " ";
	const auto build_option_args = core::tokenize(kernels_include_path+build_options, ' ');
	const size_t argc = build_option_args.size();
	vector<const char*> argv;
	for(const auto& arg : build_option_args) {
		argv.emplace_back(arg.data());
	}
	tcc_parse_args(state, (int)argc, &argv[0]);
	
	// in-memory preprocessing
	const uint8_t* code_input = (const uint8_t*)raw_code.c_str();
	tcc_in_memory_preprocess(state, code_input, raw_code.length(), &ret_code,
							 [](const char* str, void* ret) -> void {
								 *(string*)ret += str;
							 });
	
	// cleanup + return
	tcc_delete(state);
	//oclr_msg("preprocessed code: %s", ret_code);
	return ret_code;
}

string oclraster_program::create_depth_test_function(const kernel_spec& spec) const {
	if(spec.depth.depth_func == DEPTH_FUNCTION::CUSTOM) return spec.depth.custom_depth_func;
	
	string depth_test_func = "#define depth_test(incoming, current) (";
	switch(spec.depth.depth_func) {
		case DEPTH_FUNCTION::NEVER: depth_test_func += "false"; break;
		case DEPTH_FUNCTION::LESS: depth_test_func += "incoming < current"; break;
		case DEPTH_FUNCTION::EQUAL: depth_test_func += "incoming == current"; break;
		case DEPTH_FUNCTION::LESS_OR_EQUAL: depth_test_func += "incoming <= current"; break;
		case DEPTH_FUNCTION::GREATER: depth_test_func += "incoming > current"; break;
		case DEPTH_FUNCTION::NOT_EQUAL: depth_test_func += "incoming != current"; break;
		case DEPTH_FUNCTION::GREATER_OR_EQUAL: depth_test_func += "incoming >= current"; break;
		case DEPTH_FUNCTION::ALWAYS: depth_test_func += "true"; break;
		case DEPTH_FUNCTION::CUSTOM: oclr_unreachable();
	}
	depth_test_func += ")";
	return depth_test_func;
}
