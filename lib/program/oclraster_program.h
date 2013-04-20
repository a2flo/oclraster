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
#include "pipeline/image.h"
#include "pipeline/image_types.h"

// TODO: this should be in a different header
enum class PROJECTION : unsigned int {
	PERSPECTIVE,
	ORTHOGRAPHIC
};

class oclraster_program {
public:
	oclraster_program(const string& code,
					  const string entry_function = "main",
					  const string build_options = "");
	virtual ~oclraster_program();
	oclraster_program(oclraster_program& prog) = delete;
	oclraster_program& operator=(oclraster_program& prog) = delete;
	
	enum class STRUCT_TYPE : unsigned int {
		INPUT,
		OUTPUT,
		UNIFORMS,
		IMAGES,
		FRAMEBUFFER
	};
	enum class ACCESS_TYPE : unsigned int {
		READ,
		WRITE,
		READ_WRITE
	};
	enum class IMAGE_VAR_TYPE : unsigned int {
		IMAGE_1D,
		IMAGE_2D,
		IMAGE_3D,
		DEPTH_IMAGE,
		STENCIL_IMAGE
	};
	struct oclraster_struct_info {
		const STRUCT_TYPE type;
		const size2 code_pos;
		const string name;
		const string object_name;
		const vector<string> variables;
		const vector<string> variable_types;
		const vector<string> variable_specifiers;
		const bool empty;
		struct device_struct_info {
			size_t struct_size;
			vector<size_t> sizes;
			vector<size_t> offsets;
		};
		unordered_map<opencl::device_object*, const device_struct_info> device_infos;
	};
	const vector<oclraster_struct_info*>& get_structs() const;
	
	// kernel specification
	// defines the specific type and configuration of a kernel (each spec requires a different compiled kernel)
	struct kernel_spec {
		vector<image_type> image_spec;
		PROJECTION projection = PROJECTION::PERSPECTIVE;
		kernel_spec() {}
		kernel_spec(const kernel_spec& spec) : image_spec(spec.image_spec), projection(spec.projection) {}
		kernel_spec(kernel_spec&& spec) : image_spec(), projection(spec.projection) {
			this->image_spec.swap(spec.image_spec);
		}
	};
	
	//
	struct oclraster_image_info {
		vector<string> image_names;
		vector<IMAGE_VAR_TYPE> image_types;
		vector<ACCESS_TYPE> image_specifiers;
		vector<bool> is_framebuffer;
		vector<image_type> image_hints;
	};
	const oclraster_image_info& get_images() const;
	
	bool is_valid() const;
	weak_ptr<opencl::kernel_object> get_kernel(const kernel_spec spec = kernel_spec {});

protected:
	string entry_function = "main";
	string build_options = "";
	string kernel_function_name;
	
	//
	string processed_code = ""; // created once on program creation (pre-specialized processing)
	vector<kernel_spec*> compiled_kernels;
	unordered_map<kernel_spec*, weak_ptr<opencl::kernel_object>> kernels;
	weak_ptr<opencl::kernel_object> build_kernel(const kernel_spec& spec);
	
	//
	void process_program(const string& code);
	void process_image_struct(const vector<string>& variable_names,
							  const vector<string>& variable_types,
							  const vector<string>& variable_specifiers,
							  const bool is_framebuffer);
	string create_entry_function_parameters() const;
	string create_user_kernel_parameters(const kernel_spec& spec,
										 vector<string>& image_decls,
										 const bool const_output) const;
	virtual string specialized_processing(const string& code,
										  const kernel_spec& spec) = 0;
	virtual string get_fixed_entry_function_parameters() const = 0;
	virtual string get_qualifier_for_struct_type(const STRUCT_TYPE& type) const = 0;
	
	atomic<bool> valid { false };
	void invalidate(const string error_info = "");
	
	//
	vector<oclraster_struct_info*> structs;
	oclraster_image_info images;
	void generate_struct_info_cl_program(oclraster_struct_info& struct_info);
	
	//
	virtual string preprocess_code(const string& raw_code);

};

#endif
