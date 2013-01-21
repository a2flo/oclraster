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

#ifndef __OCLRASTER_OCLRASTER_PROGRAM_H__
#define __OCLRASTER_OCLRASTER_PROGRAM_H__

#include "cl/opencl.h"

class oclraster_program {
public:
	oclraster_program(const string& code, const string entry_function = "main");
	virtual ~oclraster_program();
	
	enum class STRUCT_TYPE : unsigned int {
		INPUT,
		OUTPUT,
		UNIFORMS
	};
	struct oclraster_struct_info {
		const STRUCT_TYPE type;
		const string name;
		const string object_name;
		const size2 code_pos;
		const vector<string> variables;
		const vector<string> variable_types;
		struct device_struct_info {
			size_t struct_size;
			vector<size_t> sizes;
			vector<size_t> offsets;
		};
		vector<const device_struct_info> device_infos;
	};
	
	bool is_valid() const;
	const string& get_identifier() const;

protected:
	string identifier = "";
	string entry_function = "main";
	string program_code = "";
	
	void process_program(const string& code);
	virtual void specialized_processing(const string& code) = 0;
	virtual string create_entry_function_parameters() = 0;
	
	bool valid = false;
	void invalidate(const string error_info = "");
	
	opencl::kernel_object* kernel = nullptr;
	
	//
	vector<oclraster_struct_info> structs;
	void generate_struct_info_cl_program(oclraster_struct_info& struct_info);

};

#endif
