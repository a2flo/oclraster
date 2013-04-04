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

#include "oclr_volume.h"
#include "volume.h"

// global vars, don't change these!
static bool done = false;
static event* evt = nullptr;
static camera* cam = nullptr;
static constexpr float3 cam_speeds { 0.01f, 0.1f, 0.001f };
static atomic<unsigned int> update_model { true };
static vector<transform_program*> transform_programs;
static vector<rasterization_program*> rasterization_programs;
static transform_program* volume_tp { nullptr };
static rasterization_program* volume_rp { nullptr };
static pipeline* p { nullptr };
static constexpr size_t material_count { 1 };

int main(int argc, char* argv[]) {
	// initialize oclraster
	oclraster::init(argv[0],
#if !defined(OCLRASTER_IOS)
					(const char*)"../data/"
#else
					(const char*)"/var/mobile/Documents/oclraster/"
#endif
					);
	oclraster::set_caption(APPLICATION_NAME);
	oclraster::acquire_context();
	
	// init class pointers
	evt = oclraster::get_event();
	//ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_CPU);
	ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_GPU);
	
	//
	cam = new camera();
	cam->set_position(0.8f, 0.5f, 6.0f);
	cam->set_rotation(-3.95f, 188.0f, 0.0f);
	cam->set_speed(cam_speeds.x);
	cam->set_rotation_speed(cam->get_rotation_speed() * 1.5f);
	cam->set_wasd_input(true);
	oclraster::set_camera(cam);
	
	//
	p = new pipeline();
	
	// add event handlers
	event::handler key_handler_fnctr(&key_handler);
	evt->add_event_handler(key_handler_fnctr, EVENT_TYPE::KEY_DOWN, EVENT_TYPE::KEY_UP);
	event::handler mouse_handler_fnctr(&mouse_handler);
	evt->add_internal_event_handler(mouse_handler_fnctr, EVENT_TYPE::MOUSE_RIGHT_CLICK, EVENT_TYPE::MOUSE_MOVE);
	event::handler quit_handler_fnctr(&quit_handler);
	evt->add_event_handler(quit_handler_fnctr, EVENT_TYPE::QUIT);
	event::handler kernel_reload_handler_fnctr(&kernel_reload_handler);
	evt->add_internal_event_handler(kernel_reload_handler_fnctr, EVENT_TYPE::KERNEL_RELOAD);
#if defined(OCLRASTER_IOS)
	event::handler touch_handler_fnctr(&touch_handler);
	evt->add_event_handler(touch_handler_fnctr, EVENT_TYPE::FINGER_UP, EVENT_TYPE::FINGER_DOWN, EVENT_TYPE::FINGER_MOVE);
#endif
	
	// load, compile and bind user shaders
	if(!load_programs()) return -1;
	
	// create volume and a default transfer function
	string volume_name = "c60.dat";
	if(argc > 2 && !string(argv[1]).empty() && !string(argv[2]).empty()) {
		if(string(argv[1]) == "--volume") {
			volume_name = argv[2];
		}
	}
	volume* vol = volume::from_file(oclraster::data_path("volumes/"+volume_name));
	
	uchar4* tf_data = new uchar4[256];
#if 1
	constexpr float alpha_div = 4.0f;
	for(size_t i = 0; i < 256; i++) {
		if(i < 64) tf_data[i] = uchar4(0);
		else if(i >= 64 && i < 256) {
			float f = (M_PI / 192.0f) * (float)(i - 64);
			float val = 255.0f * ((sinf(f - M_PI/2) + 1.0f) / 2.0f);
			
			if(i >= 64 && i < 96) tf_data[i] = uchar4(val, 0, 0, val / alpha_div);
			else if(i >= 96 && i < 128) tf_data[i] = uchar4(0, val, 0, val / alpha_div);
			else if(i >= 128 && i < 160) tf_data[i] = uchar4(0, 0, val, val / alpha_div);
			else tf_data[i] = uchar4(val, val, 0, val / alpha_div);
		}
		else tf_data[i] = uchar4(255);
	}
#else
	for(size_t i = 0; i < 256; i++) {
		uchar4 val;
		val.x = 255;
		val.y = i >= 100 ? 255 : 0;
		val.z = i >= 148 ? 255 : 0;
		if(i < 96) val.w = 0;
		else if(i > 122) val.w = 255;
		else {
			val.w = (unsigned char)((float(i-96) / 26.0f) * 255.0f);
		}
		tf_data[i] = val;
	}
#endif
	image* tf_texture = new image(256, 1, image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA, tf_data);
	delete [] tf_data;
	
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
			const unsigned int fps = oclraster::get_fps();
			//oclr_log("fps: %u", fps);
			stringstream caption;
			caption << APPLICATION_NAME;
			caption << " | " << fps << " FPS";
			caption << " | ~" << oclraster::get_frame_time() << "ms ";
			caption << " | Cam: " << cam->get_position();
			caption << " " << cam->get_rotation();
			oclraster::set_caption(caption.str());
		}
		
		oclraster::start_draw();
		p->bind_program(*volume_tp);
		p->bind_program(*volume_rp);
		
		// draw volume
		const float3 view_vec = oclraster::get_camera_setup().forward.normalized();
		const size_t axis = view_vec.abs().max_element_index();
		
		p->bind_buffer("index_buffer", vol->get_index_buffer(axis, view_vec[axis] < 0.0f ? 0 : 1));
		p->bind_buffer("input_attributes", vol->get_vertex_buffer(axis));
		p->bind_image("volume_texture", *vol->get_texture(axis));
		p->bind_image("tf_texture", *tf_texture);
		p->draw({ 0, vol->get_index_count(axis)-1 });
		
		p->swap();
		oclraster::stop_draw();
	}
	
	// cleanup
	delete tf_texture;
	delete vol;
	delete cam;
	
	evt->remove_event_handler(key_handler_fnctr);
	evt->remove_event_handler(mouse_handler_fnctr);
	evt->remove_event_handler(quit_handler_fnctr);
	delete p;
	oclraster::destroy();
	
	return 0;
}

bool load_programs() {
	for(auto& tp : transform_programs) {
		delete tp;
	}
	for(auto& rp : rasterization_programs) {
		delete rp;
	}
	transform_programs.clear();
	rasterization_programs.clear();
	
	string vs_str, fs_str;
	static const vector<array<string, 2>> shader_filenames {
		{{ "volume_shader_vs.cl", "volume_shader_fs.cl" }},
	};
	

	for(const auto& shader_filename : shader_filenames) {
		if(!file_io::file_to_string(oclraster::kernel_path("user/"+shader_filename[0]), vs_str)) {
			oclr_error("couldn't open vs program!");
			return false;
		}
		if(!file_io::file_to_string(oclraster::kernel_path("user/"+shader_filename[1]), fs_str)) {
			oclr_error("couldn't open fs program!");
			return false;
		}
		transform_programs.emplace_back(new transform_program(vs_str, "transform_main"));
		rasterization_programs.emplace_back(new rasterization_program(fs_str, "rasterize_main"));
	}
	volume_tp = transform_programs[0];
	volume_rp = rasterization_programs[0];
	return true;
}

bool kernel_reload_handler(EVENT_TYPE type, shared_ptr<event_object> obj oclr_unused) {
	if(type == EVENT_TYPE::KERNEL_RELOAD) {
		load_programs();
		return true;
	}
	return false;
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
			case SDLK_m:
				update_model ^= true;
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

#if defined(OCLRASTER_IOS)
bool touch_handler(EVENT_TYPE type, shared_ptr<event_object> obj oclr_unused) {
	if(type == EVENT_TYPE::FINGER_UP) {
		//const shared_ptr<finger_up_event>& touch_evt = (shared_ptr<finger_up_event>&)obj;
		//oclr_msg("finger up: %v, %u, #%u", touch_evt->position, touch_evt->pressure, touch_evt->id);
	}
	/*else if(type == EVENT_TYPE::FINGER_DOWN) {
		const shared_ptr<finger_down_event>& touch_evt = (shared_ptr<finger_down_event>&)obj;
		oclr_msg("finger down: %v, %u, #%u", touch_evt->position, touch_evt->pressure, touch_evt->id);
	}
	else if(type == EVENT_TYPE::FINGER_MOVE) {
		const shared_ptr<finger_move_event>& touch_evt = (shared_ptr<finger_move_event>&)obj;
		oclr_msg("finger move: %v -> %v, %u, #%u", touch_evt->position, touch_evt->move, touch_evt->pressure, touch_evt->id);
	}*/
	return true;
}
#endif
