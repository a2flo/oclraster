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
static float3 cam_speeds = float3(0.01f, 0.1f, 0.001f);

int main(int argc, char* argv[]) {
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
	cam->set_position(0.0f, 0.1f, -0.3f);
	cam->set_rotation(0.0f, 0.0f, 0.0f);
	cam->set_speed(cam_speeds.x);
	cam->set_wasd_input(true);
	oclraster::set_camera(cam);
	
	//
	pipeline* p = new pipeline();
	
	a2m* bunny = new a2m(oclraster::data_path("bunny.a2m"));
	
	p->_reserve_memory(std::max(8192u, bunny->get_index_buffer(0).index_count / 3));
	
	// add event handlers
	event::handler key_handler_fnctr(&key_handler);
	evt->add_event_handler(key_handler_fnctr, EVENT_TYPE::KEY_DOWN, EVENT_TYPE::KEY_UP);
	event::handler mouse_handler_fnctr(&mouse_handler);
	evt->add_internal_event_handler(mouse_handler_fnctr, EVENT_TYPE::MOUSE_RIGHT_CLICK, EVENT_TYPE::MOUSE_MOVE);
	event::handler quit_handler_fnctr(&quit_handler);
	evt->add_event_handler(quit_handler_fnctr, EVENT_TYPE::QUIT);
	
	///// testing
	oclraster_program vs_prog(oclraster::kernel_path("user/simple_shader_vs.cl"));
	oclraster_program fs_prog(oclraster::kernel_path("user/simple_shader_fs.cl"));
	/////
	
	// init done
	oclraster::release_context();
	
	// main loop
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
		
		// draw something
		p->draw(bunny->get_vertex_buffer(), bunny->get_index_buffer(0));
		
		/*float3 cur_pos = cam->get_position();
		for(unsigned int i = 0; i < 3; i++) {
			cam->set_position(cur_pos.x + 0.5f * float(i), cur_pos.y, cur_pos.z);
			oclraster::run_camera();
			p->draw(bunny->get_vertex_buffer(), bunny->get_index_buffer(0));
		}
		cam->set_position(cur_pos);
		oclraster::run_camera();*/
		
		p->stop();
		oclraster::stop_draw();
	}
	
	delete bunny;
	delete cam;
	
	// cleanup
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

bool mouse_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::MOUSE_RIGHT_CLICK) {
		cam->set_mouse_input(cam->get_mouse_input() ^ true);
		// TODO: switch cam input
		//oclraster::set_cursor_visible(oclraster::get_cursor_visible() ^ true);
	}
	return true;
}

bool quit_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	done = true;
	return true;
}
