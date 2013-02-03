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

#include "oclr_simple.h"

// global vars, don't change these!
static bool done = false;
static event* evt = nullptr;
static camera* cam = nullptr;
static constexpr float3 cam_speeds { 0.01f, 0.1f, 0.001f };

int main(int argc oclr_unused, char* argv[]) {
	// initialize oclraster
	oclraster::init(argv[0], (const char*)"../data/");
	oclraster::set_caption(APPLICATION_NAME);
	oclraster::acquire_context();
	
	// init class pointers
	evt = oclraster::get_event();
	ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_CPU);
	//ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_GPU);
	
	//
	cam = new camera();
#if 1
	cam->set_position(0.0f, 0.1f, -0.3f);
	//cam->set_position(1.12157e-12f, 0.1f, -2.22f);
	cam->set_rotation(0.0f, 0.0f, 0.0f);
#else
	cam->set_position(10.0f, 5.0f, -10.0f);
	cam->set_rotation(-20.0f, -45.0f, 0.0f);
#endif
	cam->set_speed(cam_speeds.x);
	cam->set_wasd_input(true);
	oclraster::set_camera(cam);
	
	//
	pipeline* p = new pipeline();
	
	a2m* bunny = new a2m(oclraster::data_path("bunny.a2m"));
	//a2m* bunny = new a2m(oclraster::data_path("cube.a2m"));
	
	p->_reserve_memory(std::max(8192u, bunny->get_index_buffer(0).index_count / 3));
	
	// add event handlers
	event::handler key_handler_fnctr(&key_handler);
	evt->add_event_handler(key_handler_fnctr, EVENT_TYPE::KEY_DOWN, EVENT_TYPE::KEY_UP);
	event::handler mouse_handler_fnctr(&mouse_handler);
	evt->add_internal_event_handler(mouse_handler_fnctr, EVENT_TYPE::MOUSE_RIGHT_CLICK, EVENT_TYPE::MOUSE_MOVE);
	event::handler quit_handler_fnctr(&quit_handler);
	evt->add_event_handler(quit_handler_fnctr, EVENT_TYPE::QUIT);
	
	// load, compile and bind user shaders
	stringstream vs_buffer, fs_buffer;
	if(!file_io::file_to_buffer(oclraster::kernel_path("user/simple_shader_vs.cl"), vs_buffer)) {
		oclr_error("couldn't open vs program!");
		return -1;
	}
	if(!file_io::file_to_buffer(oclraster::kernel_path("user/simple_shader_fs.cl"), fs_buffer)) {
		oclr_error("couldn't open fs program!");
		return -1;
	}
	transform_program vs_prog(vs_buffer.str(), "transform_main");
	rasterization_program fs_prog(fs_buffer.str(), "rasterize_main");
	p->bind_program(vs_prog);
	p->bind_program(fs_prog);
	
	// create / ref buffers
	const opencl::buffer_object& index_buffer = *bunny->get_index_buffer(0).buffer;
	const opencl::buffer_object& input_attributes = *bunny->get_vertex_buffer().buffer;
	
	matrix4f model_matrix { matrix4f() };
	opencl::buffer_object* tp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(matrix4f),
																   (void*)&model_matrix);
	
	float light_pos = M_PI, light_dist = 10.0f;
	struct __attribute__((packed, aligned(16))) rp_uniforms {
		float4 camera_position;
		float4 light_position; // .w = light radius ^ 2
		float4 light_color;
	} rasterize_uniforms {
		float4(oclraster::get_camera_setup().position, 1.0f),
		float4(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, 16.0f*16.0f),
		float4(0.0f, 0.3f, 0.7f, 1.0f)
	};
	opencl::buffer_object* rp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(rp_uniforms),
																   (void*)&rasterize_uniforms);
	
	// init done
	oclraster::release_context();
	
	// main loop
	float model_rotation = 0.0f;
	float3 model_scale { 1.0f, 1.0f, 1.0f };
	float3 target_scale { model_scale };
	static constexpr float model_scale_range = 0.4f, model_scale_step = 0.01f;
	while(!done) {
		// event handling
		evt->handle_events();
		
#if !defined(OCLRASTER_DEBUG) && 0
		// stop drawing if window is inactive
		if(!(SDL_GetWindowFlags(oclraster::get_window()) & SDL_WINDOW_INPUT_FOCUS)) {
			SDL_Delay(20);
			continue;
		}
#endif
		
		// set caption (app name and fps count)
		if(oclraster::is_new_fps_count()) {
			static stringstream caption;
			caption << APPLICATION_NAME;
			caption << " | " << oclraster::get_fps() << " FPS";
			caption << " | ~" << oclraster::get_frame_time() << "ms ";
			caption << " | Cam: " << cam->get_position();
			oclraster::set_caption(caption.str().c_str());
			core::reset(caption);
		}
		
		oclraster::start_draw();
		p->start();
		
		//cout << endl << endl << " ### frame ### " << endl << endl;
		
		// update uniforms
		model_matrix = matrix4f().rotate_y(model_rotation);
		model_rotation += 1.0f;
		model_rotation = core::wrap(model_rotation, 360.0f);
		
		const float3 scale_diff = (model_scale - target_scale).abs();
		if((scale_diff <= float3(model_scale_step * 2.0f)).all()) {
			target_scale.x = 1.0f + core::rand(-model_scale_range, model_scale_range);
			target_scale.y = 1.0f + core::rand(-model_scale_range, model_scale_range);
			target_scale.z = 1.0f + core::rand(-model_scale_range, model_scale_range);
		}
		else {
			for(unsigned int i = 0; i < 3; i++) {
				if(scale_diff[i] <= (model_scale_step * 2.0f)) continue;
				model_scale[i] += model_scale_step * (model_scale[i] <= target_scale[i] ? 1.0f : -1.0f);
			}
		}
		model_matrix.scale(model_scale.x, model_scale.y, model_scale.z);
		//ocl->write_buffer(tp_uniforms_buffer, &model_matrix);
		
		light_pos -= 0.25f;
		rasterize_uniforms.light_position.set(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, 16.0f*16.0f);
		static constexpr float color_step_range = 0.2f;
		rasterize_uniforms.light_color.x += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.y += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.z += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.clamp(0.0f, 1.0f);
		ocl->write_buffer(rp_uniforms_buffer, &rasterize_uniforms);
		
		// draw something
		p->bind_buffer("index_buffer", index_buffer);
		p->bind_buffer("input_attributes", input_attributes);
		p->bind_buffer("tp_uniforms", *tp_uniforms_buffer);
		p->bind_buffer("rp_uniforms", *rp_uniforms_buffer);
		p->draw({0, bunny->get_index_count(0)-1});
		
		p->stop();
		oclraster::stop_draw();
	}
	
	// cleanup
	delete bunny;
	delete cam;
	
	ocl->delete_buffer(tp_uniforms_buffer);
	ocl->delete_buffer(rp_uniforms_buffer);
	
	evt->remove_event_handler(key_handler_fnctr);
	evt->remove_event_handler(mouse_handler_fnctr);
	evt->remove_event_handler(quit_handler_fnctr);
	delete p;
	oclraster::destroy();
	
	return 0;
}

bool key_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::KEY_DOWN) {
		const shared_ptr<key_down_event>& key_evt = (shared_ptr<key_down_event>&)obj;
		switch(key_evt->key) {
			case SDLK_LCTRL:
				cam->set_speed(cam_speeds.z);
				break;
			case SDLK_LSHIFT:
				cam->set_speed(cam_speeds.y);
				break;
			default: return false;
		}
	}
	else if(type == EVENT_TYPE::KEY_UP) {
		const shared_ptr<key_up_event>& key_evt = (shared_ptr<key_up_event>&)obj;
		switch(key_evt->key) {
			case SDLK_ESCAPE:
				done = true;
				break;
			case SDLK_LCTRL:
			case SDLK_LSHIFT:
				cam->set_speed(cam_speeds.x);
				break;
			case SDLK_F19:
			case SDLK_0:
				oclraster::reload_kernels();
				break;
			default: return false;
		}
	}
	return true;
}

bool mouse_handler(EVENT_TYPE type, shared_ptr<event_object> obj oclr_unused) {
	if(type == EVENT_TYPE::MOUSE_RIGHT_CLICK) {
		cam->set_mouse_input(cam->get_mouse_input() ^ true);
		// TODO: switch cam input
		//oclraster::set_cursor_visible(oclraster::get_cursor_visible() ^ true);
	}
	return true;
}

bool quit_handler(EVENT_TYPE type oclr_unused, shared_ptr<event_object> obj oclr_unused) {
	done = true;
	return true;
}
