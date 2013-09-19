/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
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

#ifndef __OCLRASTER_SUPPORT_SHADER_HPP__
#define __OCLRASTER_SUPPORT_SHADER_HPP__

#include <oclraster_support/global.hpp>
#include <oclraster/pipeline/pipeline.hpp>
#include <oclraster/program/transform_program.hpp>
#include <oclraster/program/rasterization_program.hpp>

class shader_helper {
public:
	//
	shader_helper() = delete;
	~shader_helper() = delete;
	static void init(pipeline* p);
	static void destroy();
	
	struct oclr_shader {
		unordered_map<string, pair<transform_program*, rasterization_program*>> shaders;
		atomic<bool> compilation_failure { false };
		atomic<unsigned int> compiled_shaders { 0 };
		oclr_shader(const string& tp_filename,
					const string& rp_filename,
					// <option name, build options>
					initializer_list<pair<string, string>> options);
		~oclr_shader();
		void use(const string option = "#",
				 const bool default_transform_uniforms = true,
				 const opencl::buffer_object* transform_uniforms = nullptr);
		template <typename T> void set_uniforms(const T& uniforms) {
			ocl->write_buffer(primitives_rp_uniforms, &uniforms, 0, sizeof(T));
			oclr_pipeline->bind_buffer("rp_uniforms", *primitives_rp_uniforms);
		}
	};
	
	//
	static shader_helper::oclr_shader* simple_shd;
	static shader_helper::oclr_shader* gradient_shd;
	static shader_helper::oclr_shader* texture_shd;
	static shader_helper::oclr_shader* font_shd;
	static shader_helper::oclr_shader* blend_shd;
	
protected:
	static pipeline* oclr_pipeline;
	
	static opencl::buffer_object* primitives_tp_uniforms;
	static opencl::buffer_object* primitives_rp_uniforms;
	
	static void reload_shaders();
	static event::handler evt_handler;
	static bool event_handler(EVENT_TYPE type, shared_ptr<event_object> obj);
	
};

// uniforms of built-in shaders
#define primitives_rp_uniform_buffer_size (256)
oclraster_struct simple_shd_uniforms {
	float4 color;
};
static_assert(sizeof(simple_shd_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");

oclraster_struct gradient_shd_uniforms {
	array<float4, 4> gradients; // -> max gradient colors: 4
	float4 stops;
	float4 extent;
};
static_assert(sizeof(gradient_shd_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");

oclraster_struct texture_shd_uniforms {
	float4 extent;
	float4 orientation;
	float draw_depth;
};
static_assert(sizeof(texture_shd_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");
oclraster_struct texture_madd_color_shd_uniforms {
	float4 mul_color;
	float4 add_color;
	
	float4 extent;
	float4 orientation;
	float draw_depth;
};
static_assert(sizeof(texture_madd_color_shd_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");
oclraster_struct texture_gradient_shd_uniforms {
	float4 mul_color;
	float4 add_color;
	
	float4 gradient_mul_interpolator;
	float4 gradient_add_interpolator;
	array<float4, 4> gradients; // -> max gradient colors: 4
	float4 stops;
	
	float4 extent;
	float4 orientation;
	float draw_depth;
};
static_assert(sizeof(texture_gradient_shd_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");
oclraster_struct font_shd_rp_uniforms {
	float4 font_color;
};
static_assert(sizeof(font_shd_rp_uniforms) <= (primitives_rp_uniform_buffer_size * sizeof(float)),
			  "invalid ubo size");

#endif
