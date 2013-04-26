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
enum class DEPTH_FUNCTION : unsigned int {
	NEVER,
	LESS,
	EQUAL,
	LESS_OR_EQUAL,
	GREATER,
	NOT_EQUAL,
	GREATER_OR_EQUAL,
	ALWAYS,
	CUSTOM
};
struct depth_state {
	DEPTH_FUNCTION depth_func;
	string custom_depth_func;
	bool depth_test;
	bool depth_override;
	
	depth_state(const depth_state& state) :
	depth_func(state.depth_func), custom_depth_func(state.custom_depth_func), depth_test(state.depth_test), depth_override(state.depth_override) {}
	depth_state(const DEPTH_FUNCTION depth_func_ = DEPTH_FUNCTION::LESS,
				const string custom_depth_func_ = "",
				const bool depth_test_ = true,
				const bool depth_override_ = false) :
	depth_func(depth_func_), custom_depth_func(depth_func_ == DEPTH_FUNCTION::CUSTOM ? custom_depth_func_ : ""),
	depth_test(depth_test_), depth_override(depth_override_) {}
	
	bool operator==(const depth_state& state) const {
		if(state.depth_func != depth_func) return false;
		if(state.depth_test != depth_test) return false;
		if(state.depth_override != depth_override) return false;
		if(state.depth_func == DEPTH_FUNCTION::CUSTOM && state.custom_depth_func != custom_depth_func) return false;
		return true;
	}
	bool operator!=(const depth_state& state) const {
		return !(*this == state);
	}
};

class oclraster_program {
public:
	// kernel specification
	// defines the specific type and configuration of a kernel (each spec requires a different compiled kernel)
	struct kernel_spec {
		vector<image_type> image_spec;
		PROJECTION projection;
		depth_state depth;
		
		kernel_spec(const kernel_spec& spec) :
		image_spec(spec.image_spec), projection(spec.projection), depth(spec.depth) {}
		kernel_spec(kernel_spec&& spec) : image_spec(), projection(spec.projection), depth(spec.depth) {
			this->image_spec.swap(spec.image_spec);
		}
		kernel_spec(const vector<image_type> image_spec_ = vector<image_type> {},
					const PROJECTION projection_ = PROJECTION::PERSPECTIVE,
					const DEPTH_FUNCTION depth_func_ = DEPTH_FUNCTION::LESS,
					const string custom_depth_func_ = "",
					const bool depth_test_ = true,
					const bool depth_override_ = false) :
		image_spec(image_spec_), projection(projection_),
		depth(depth_func_, depth_func_ == DEPTH_FUNCTION::CUSTOM ? custom_depth_func_ : "",
			  depth_test_, depth_override_) {}
		
		bool operator==(const kernel_spec& spec) const {
			if(spec.projection != projection) return false;
			if(spec.depth != depth) return false;
			if(spec.image_spec.size() != spec.image_spec.size()) return false;
			for(size_t i = 0, spec_size = image_spec.size(); i < spec_size; i++) {
				if(image_spec[i] != spec.image_spec[i]) return false;
			}
			return true;
		}
		bool operator!=(const kernel_spec& spec) const {
			return !(*this == spec);
		}
	};
	
	//
	oclraster_program(const string& code,
					  const string entry_function = "main",
					  const string build_options = "",
					  const kernel_spec default_spec = kernel_spec {});
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
	void process_program(const string& code, const kernel_spec default_spec);
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
