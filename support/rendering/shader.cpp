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


#include "shader.hpp"
#include <floor/threading/task.hpp>
#include <oclraster/program/oclraster_program.hpp>

pipeline* shader_helper::oclr_pipeline = nullptr;

shader_helper::oclr_shader* shader_helper::simple_shd { nullptr };
shader_helper::oclr_shader* shader_helper::gradient_shd { nullptr };
shader_helper::oclr_shader* shader_helper::texture_shd { nullptr };
shader_helper::oclr_shader* shader_helper::font_shd { nullptr };
shader_helper::oclr_shader* shader_helper::blend_shd { nullptr };

static constexpr size_t primitives_tp_uniform_buffer_size = 16; // they all use a mat4
opencl::buffer_object* shader_helper::primitives_tp_uniforms = nullptr;
opencl::buffer_object* shader_helper::primitives_rp_uniforms = nullptr;

event::handler shader_helper::evt_handler(&shader_helper::event_handler);

//
shader_helper::oclr_shader::oclr_shader(const string& tp_filename,
								  const string& rp_filename,
								  // <option name, build options>
								  initializer_list<pair<string, string>> options) {
	// load
	string tp_str = "", rp_str = "";
	if(!file_io::file_to_string(floor::kernel_path("support/"+tp_filename), tp_str)) {
		log_error("couldn't open tp program \"%s\"!", tp_filename);
		return;
	}
	if(!file_io::file_to_string(floor::kernel_path("support/"+rp_filename), rp_str)) {
		log_error("couldn't open rp program \"%s\"!", rp_filename);
		return;
	}
	
	// compile
	for(const auto& option : options) {
		shaders.emplace(option.first, pair<transform_program*, rasterization_program*> { nullptr, nullptr });
	}
	for(const auto& option : options) {
		log_msg("compiling %s / %s ...", option.first, option.second);
		pair<transform_program*, rasterization_program*>* shader = &shaders[option.first];
		static const oclraster_program::kernel_spec default_spec {
			{},
			PROJECTION::ORTHOGRAPHIC,
			DEPTH_FUNCTION::LESS_OR_EQUAL
		};
		task::spawn([this, tp_str, rp_str, shader, option]() {
			shader->first = new transform_program(tp_str, "gfx2d_transform", "-D"+option.second, default_spec);
			shader->second = new rasterization_program(rp_str, "gfx2d_rasterization", "-D"+option.second, default_spec);
			if(!shader->first->is_valid()) compilation_failure = true;
			if(!shader->second->is_valid()) compilation_failure = true;
			compiled_shaders++;
		});
	}
}

shader_helper::oclr_shader::~oclr_shader() {
	for(auto& shd : shaders) {
		if(shd.second.first != nullptr) delete shd.second.first;
		if(shd.second.second != nullptr) delete shd.second.second;
	}
	shaders.clear();
}

void shader_helper::oclr_shader::use(const string option,
									 const bool default_transform_uniforms,
									 const opencl::buffer_object* transform_uniforms) {
	const auto shd_iter = shaders.find(option);
	if(shd_iter == shaders.end()) {
		log_error("shader option \"%s\" doesn't exist!", option);
		return;
	}
	oclr_pipeline->bind_program(*shd_iter->second.first);
	oclr_pipeline->bind_program(*shd_iter->second.second);
	
	if(default_transform_uniforms) {
		oclr_pipeline->bind_buffer("tp_uniforms", *primitives_tp_uniforms);
	}
	else if(transform_uniforms != nullptr) {
		oclr_pipeline->bind_buffer("tp_uniforms", *transform_uniforms);
	}
}

//
void shader_helper::init(pipeline* p_) {
	oclr_pipeline = p_;
	
	// create a large enough uniform buffer
	matrix4f tp_mvm; // identity matrix for now
	primitives_tp_uniforms = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
												opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
												opencl::BUFFER_FLAG::INITIAL_COPY,
												sizeof(float) * primitives_tp_uniform_buffer_size,
												&tp_mvm);
	primitives_rp_uniforms = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
												opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
												sizeof(float) * primitives_rp_uniform_buffer_size);
	
	//
	floor::get_event()->add_internal_event_handler(evt_handler, EVENT_TYPE::KERNEL_RELOAD);
	reload_shaders();
}

void shader_helper::destroy() {
	floor::get_event()->remove_event_handler(evt_handler);
	
	if(primitives_tp_uniforms != nullptr) {
		ocl->delete_buffer(primitives_tp_uniforms);
	}
	if(primitives_rp_uniforms != nullptr) {
		ocl->delete_buffer(primitives_rp_uniforms);
	}
}

void shader_helper::reload_shaders() {
	if(simple_shd != nullptr) {
		delete simple_shd;
		simple_shd = nullptr;
	}
	if(gradient_shd != nullptr) {
		delete gradient_shd;
		gradient_shd = nullptr;
	}
	if(texture_shd != nullptr) {
		delete texture_shd;
		texture_shd = nullptr;
	}
	if(font_shd != nullptr) {
		delete font_shd;
		font_shd = nullptr;
	}
	if(blend_shd != nullptr) {
		delete blend_shd;
		blend_shd = nullptr;
	}
	//
	const auto timer_start = SDL_GetPerformanceCounter();
	simple_shd = new oclr_shader("gfx2d_simple.cl", "gfx2d_simple.cl", {
		{ "#", "SIMPLE_STD" },
	});
	gradient_shd = new oclr_shader("gfx2d_gradient.cl", "gfx2d_gradient.cl", {
		{ "gradient_horizontal", "GRADIENT_HORIZONTAL" },
		{ "gradient_vertical", "GRADIENT_VERTICAL" },
		{ "gradient_diagonal_lr", "GRADIENT_DIAGONAL_LR" },
		{ "gradient_diagonal_rl", "GRADIENT_DIAGONAL_RL" },
	});
	texture_shd = new oclr_shader("gfx2d_texture.cl", "gfx2d_texture.cl", {
		{ "#", "TEXTURE_STD" },
		{ "passthrough", "TEXTURE_PASSTHROUGH" },
		{ "madd_color", "TEXTURE_MADD_COLOR" },
		{ "gradient_horizontal", "TEXTURE_GRADIENT_HORIZONTAL" },
		{ "gradient_vertical", "TEXTURE_GRADIENT_VERTICAL" },
		{ "gradient_diagonal_lr", "TEXTURE_GRADIENT_DIAGONAL_LR" },
		{ "gradient_diagonal_rl", "TEXTURE_GRADIENT_DIAGONAL_RL" },
	});
	font_shd = new oclr_shader("gfx2d_font.cl", "gfx2d_font.cl", {
		{ "#", "FONT_STD" },
	});
	blend_shd = new oclr_shader("gfx2d_blend.cl", "gfx2d_blend.cl", {
		{ "#", "BLEND_STD" },
	});
	while(simple_shd->compiled_shaders != simple_shd->shaders.size()) {
		this_thread::yield();
	}
	while(gradient_shd->compiled_shaders != gradient_shd->shaders.size()) {
		this_thread::yield();
	}
	while(texture_shd->compiled_shaders != texture_shd->shaders.size()) {
		this_thread::yield();
	}
	while(font_shd->compiled_shaders != font_shd->shaders.size()) {
		this_thread::yield();
	}
	while(blend_shd->compiled_shaders != blend_shd->shaders.size()) {
		this_thread::yield();
	}
	const auto timer_end = SDL_GetPerformanceCounter();
	log_msg("compilation time: %fs (%u)",
			 float(timer_end - timer_start) / float(SDL_GetPerformanceFrequency()),
			 (timer_end - timer_start));
}

bool shader_helper::event_handler(EVENT_TYPE type, shared_ptr<event_object> obj floor_unused) {
	if(type == EVENT_TYPE::KERNEL_RELOAD) {
		shader_helper::reload_shaders();
	}
	return true;
}
