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

#ifndef __OCLRASTER_IOS_HELPER_HPP__
#define __OCLRASTER_IOS_HELPER_HPP__

#include "core/gl_support.hpp"
#include "core/cpp_headers.hpp"
#include <regex>

struct shader_object {
	struct internal_shader_object {
		unsigned int program;
		unsigned int vertex_shader;
		unsigned int fragment_shader;
		
		struct shader_variable {
			size_t location;
			size_t size;
			size_t type;
			shader_variable(size_t location_, size_t size_, GLenum type_) : location(location_), size(size_), type(type_) {}
		};
		unordered_map<string, shader_variable> uniforms;
		unordered_map<string, shader_variable> attributes;
		unordered_map<string, size_t> samplers;
		
		internal_shader_object() : program(0), vertex_shader(0), fragment_shader(0), uniforms(), attributes(), samplers() {}
		~internal_shader_object() {
			// TODO: delete shaders?
			uniforms.clear();
			attributes.clear();
			samplers.clear();
		}
	};
	const string name;
	internal_shader_object program;
	
	shader_object(const string& shd_name) : name(shd_name) {}
	~shader_object() {}
};

class ios_helper {
public:
	static void* get_eagl_sharegroup();
	static void compile_shaders();
	static shader_object* get_shader(const string& name);
	
};

#endif
