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

oclraster_program::oclraster_program(const string& code oclr_unused,
									 const string entry_function_) :
entry_function(entry_function_) {
}

oclraster_program::~oclraster_program() {
	if(kernel != nullptr && ocl != nullptr) {
		ocl->delete_kernel(kernel);
	}
}

void oclraster_program::process_program(const string& code) {
	static const array<const pair<const char*, const STRUCT_TYPE>, 3> oclraster_struct_types {
		{
			{ u8"oclraster_in", STRUCT_TYPE::INPUT },
			{ u8"oclraster_out", STRUCT_TYPE::OUTPUT },
			{ u8"oclraster_uniforms", STRUCT_TYPE::UNIFORMS },
		}
	};
	
	// current oclraster_struct grammar limitation/requirements:
	// * no preprocessor (this would require a compiler)
	// * no interior structs/unions (TODO: possibly allow this)
	// * no multi-variable declarations (e.g. "float x, y, z;")
	// * no user defined types are allowed (TODO: for now?)
	// * use of any oclraster_struct specifier in other places is disallowed (no typedefs, comments, ...)
	// * otherwise standard c
	//
	// TODO: __attribute__ handling
	//
	// example:
	// oclraster_in vertex_input {
	// 		float4 vertex;
	// 		float4 normal;
	// 		float2 tex_coord;
	// };
	//
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
				vector<string> variable_names, variable_types;
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
					const string var_type = var_decl.substr(0, name_start_pos);
					//oclr_msg("type: >%s<", var_type);
					variable_names.emplace_back(var_name);
					variable_types.emplace_back(var_type);
					
					// continue
					last_semicolon_pos = semicolon_pos+1;
				}
				
				// create info struct
				structs.push_back({
					type.second,
					struct_name,
					object_name,
					size2(struct_pos, end_semicolon_pos+1),
					std::move(variable_names),
					std::move(variable_types),
					{}
				});
				
				// continue
				struct_pos++;
			}
		}
		
		// process found structs
		for(auto& oclr_struct : structs) {
			generate_struct_info_cl_program(oclr_struct);
		}
		
		// build entry function parameter string
		const string entry_function_params = create_entry_function_parameters();
		
		// check if entry function exists, and if so, replace it with a modified function name
		const regex rx_entry_function("("+entry_function+")\\s*\\(\\s*\\)");
		if(!regex_search(code, rx_entry_function)) {
			throw oclraster_exception("entry function \""+entry_function+"\" not found!");
		}
		string processed_code = regex_replace(code, rx_entry_function,
											  "_oclraster_user_"+entry_function+"("+entry_function_params+")");
		
		// recreate structs (in reverse, so that the offsets stay valid)
		for(auto iter = structs.rbegin(); iter != structs.rend(); iter++) {
			processed_code.erase(iter->code_pos.x, iter->code_pos.y - iter->code_pos.x);
			
			string struct_code = "";
			switch(iter->type) {
				case STRUCT_TYPE::INPUT:
					struct_code += "oclraster_in";
					break;
				case STRUCT_TYPE::OUTPUT:
					struct_code += "oclraster_out";
					break;
				case STRUCT_TYPE::UNIFORMS:
					struct_code += "oclraster_uniforms";
					break;
			}
			struct_code += " {\n";
			for(size_t var_index = 0; var_index < iter->variables.size(); var_index++) {
				struct_code += iter->variable_types[var_index] + " " + iter->variables[var_index] + ";\n";
			}
			struct_code += "} " + iter->name + ";\n";
			processed_code.insert(iter->code_pos.x, struct_code);
		}
		
		// and finally: call the specialized processing function of inheriting classes/programs
		// note: this should inject the user code into their respective code templates
		specialized_processing(processed_code);
		
		// compile
		identifier = "USER_PROGRAM."+ull2string(SDL_GetPerformanceCounter());
		kernel = ocl->add_kernel_src(identifier, program_code, "_oclraster_program");
	}
	catch(oclraster_exception& ex) {
		invalidate(ex.what());
	}
}

void oclraster_program::generate_struct_info_cl_program(oclraster_struct_info& struct_info) {
	static const string kernel_header = "#include \"oclr_global.h\"\n#include \"oclr_matrix.h\"\n";
	static const string kernel_start = "kernel void struct_info(global int* info_buffer) {\nint index = 0;\n";
	static const string kernel_end = "}";
	
	string kernel_code = kernel_header;
	
	// struct decl
	kernel_code += "typedef struct __attribute__((packed, aligned(16))) {\n";
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
	const string unique_identifier = "STRUCT_INFO."+ull2string(SDL_GetPerformanceCounter());
	opencl::kernel_object* kernel_obj = ocl->add_kernel_src(unique_identifier, kernel_code, "struct_info");
	
	const size_t info_buffer_size = 1 + struct_info.variables.size() * 2;
	int* info_buffer_results = new int[info_buffer_size];
	
	auto active_device = ocl->get_active_device();
	const auto& devices = ocl->get_devices();
	//struct_info.device_infos.resize(devices.size());
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
		struct_info.device_infos.push_back(dev_info);
		
		ocl->delete_buffer(info_buffer);
	}
	ocl->set_active_device(active_device->type);
	
	// cleanup
	delete [] info_buffer_results;
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

const string& oclraster_program::get_identifier() const {
	return identifier;
}

const vector<oclraster_program::oclraster_struct_info>& oclraster_program::get_structs() const {
	return structs;
}
