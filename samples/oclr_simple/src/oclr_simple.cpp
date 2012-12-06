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
static opencl_base* ocl = nullptr;
static float3 position(0.0f, 0.1f, -0.3f);
//static float3 position(0.0f, 0.0f, 0.0f);
static float3 cam_forward(0.0f, 0.0f, 1.0f);
static float3 cam_up(0.0f, 1.0f, 0.0f);
static bool cam_mouse_input = false;
static float2 cam_rotation(0.0f, 0.0f);
static int cam_ignore_next_rotation = 2;
static double cam_rotation_speed = 100.0f;
static float cam_speed = 0.1f;

int main(int argc, char* argv[]) {
	// initialize oclraster
	oclraster::init(argv[0], (const char*)"../data/");
	oclraster::set_caption(APPLICATION_NAME);
	oclraster::acquire_context();
	
	oclraster::setup_camera(position,
							float3(0.0f, 0.0f, 1.0f),
							float3(0.0f, 1.0f, 0.0f));
	
	// init class pointers
	evt = oclraster::get_event();
	ocl = oclraster::get_opencl();
	//ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_CPU);
	ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_GPU);
	
	//
	pipeline* p = new pipeline();
	transform_stage::vertex_buffer vb;
	transform_stage::index_buffer ib;
	transform_stage::constant_buffer cb;
	
	const unsigned int num_triangles = 1;
	const unsigned int num_vertices = num_triangles * 3;
	const unsigned int num_indices = num_triangles * 3;
	struct vertex_data {
		float4 vertex;
		float4 normal;
		float4 binormal;
		float4 tangent;
		//float2 tex_coord;
	};
	vertex_data* vertices = new vertex_data[num_vertices];
	unsigned int* indices = new unsigned int[num_indices];
	const float3 offset(0.0f, 0.0f, 2.0f);
	{
		vertices[0].vertex.set(-0.1f, 0.0f, offset.z, 1.0f);
		vertices[1].vertex.set(0.0f, 0.1f, offset.z, 1.0f);
		vertices[2].vertex.set(0.1f, 0.0f, offset.z, 1.0f);
		
		for(unsigned int i = 0; i < num_indices; i++) {
			indices[i] = i;
		}
	}
	vb.buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ | opencl::BUFFER_FLAG::INITIAL_COPY,
								   sizeof(vertex_data) * num_vertices,
								   &vertices[0]);
	vb.elements.emplace_back("vertex", sizeof(float4));
	vb.elements.emplace_back("normal", sizeof(float4));
	vb.elements.emplace_back("binormal", sizeof(float4));
	vb.elements.emplace_back("tangent", sizeof(float4));
	//vb.elements.emplace_back("tex_coord", sizeof(float2));
	ib.buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ | opencl::BUFFER_FLAG::INITIAL_COPY,
								   sizeof(unsigned int) * num_indices,
								   &indices[0]);
	ib.index_count = num_indices;
	delete [] vertices;
	delete [] indices;
	
	pipeline::draw_flags draw_flags;
	draw_flags.depth_test = 1;
	draw_flags.transformed_primitive_size = 4 * sizeof(float4);
	//printf("sizes: %u %u %u\n", sizeof(vertex_data), 0, draw_flags.transformed_primitive_size);
	
	a2m* bunny = new a2m(oclraster::data_path("bunny.a2m"));
	
	p->reserve_memory(std::max(8192u, bunny->get_index_buffer(0).index_count / 3));
	
	// add event handlers
	event::handler key_handler_fnctr(&key_handler);
	evt->add_event_handler(key_handler_fnctr, EVENT_TYPE::KEY_DOWN, EVENT_TYPE::KEY_UP);
	event::handler mouse_handler_fnctr(&mouse_handler);
	evt->add_internal_event_handler(mouse_handler_fnctr, EVENT_TYPE::MOUSE_RIGHT_CLICK, EVENT_TYPE::MOUSE_MOVE);
	event::handler quit_handler_fnctr(&quit_handler);
	evt->add_event_handler(quit_handler_fnctr, EVENT_TYPE::QUIT);
	
	// init done
	oclraster::release_context();
	
	// main loop
	while(!done) {
		// event handling
		evt->handle_events();
		
#if !defined(OCLRASTER_DEBUG)
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
			caption << " | Cam: " << -oclraster::get_position();
			oclraster::set_caption(caption.str().c_str());
			core::reset(caption);
		}
		
		oclraster::start_draw();
		
		oclraster::setup_camera(position, cam_forward, cam_up);
		
		// draw something
		//p->draw(vb, ib, draw_flags);
		p->draw(bunny->get_vertex_buffer(), bunny->get_index_buffer(0), draw_flags);
		
		oclraster::stop_draw();
		//break;
	}
	
	delete bunny;
	
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
			case SDLK_w:
				position.x += (float)sin(cam_rotation.y * _PIDIV180) * cam_speed;
				position.y += (float)sin(cam_rotation.x * _PIDIV180) * cam_speed;
				position.z += (float)cos(cam_rotation.y * _PIDIV180) * cam_speed;
				break;
			case SDLK_a:
				position.x += (float)sin((cam_rotation.y - 90.0f) * _PIDIV180) * cam_speed;
				position.z -= (float)cos((cam_rotation.y - 90.0f) * _PIDIV180) * cam_speed;
				break;
			case SDLK_s:
				position.x -= (float)sin(cam_rotation.y * _PIDIV180) * cam_speed;
				position.y -= (float)sin(cam_rotation.x * _PIDIV180) * cam_speed;
				position.z -= (float)cos(cam_rotation.y * _PIDIV180) * cam_speed;
				break;
			case SDLK_d:
				position.x -= (float)sin((cam_rotation.y - 90.0f) * _PIDIV180) * cam_speed;
				position.z += (float)cos((cam_rotation.y - 90.0f) * _PIDIV180) * cam_speed;
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
		cam_mouse_input ^= true;
		// TODO: switch cam input
		//oclraster::set_cursor_visible(oclraster::get_cursor_visible() ^ true);
		
		// grab input
		SDL_SetWindowGrab(oclraster::get_window(), (cam_mouse_input ? SDL_TRUE : SDL_FALSE));
		
#if defined(__APPLE__)
		// this effictively calls CGAssociateMouseAndMouseCursorPosition (which will lock the cursor to the window)
		// and subsequently handles all mouse moves in relative/delta mode
		SDL_SetRelativeMouseMode(cam_mouse_input ? SDL_TRUE : SDL_FALSE);
		
		// this fixes some weird mouse positioning when switching from grab to non-grab mode
		if(!cam_mouse_input) {
			const float2 center_point(float2(oclraster::get_width(), oclraster::get_height()) * 0.5f);
			SDL_WarpMouseInWindow(oclraster::get_window(), roundf(center_point.x), roundf(center_point.y));
		}
#endif
		cam_ignore_next_rotation = 2;
	}
	else if(type == EVENT_TYPE::MOUSE_MOVE && cam_mouse_input) {
		// calculate the rotation via the current mouse cursor position
		int cursor_pos_x = 0;
		int cursor_pos_y = 0;
		
		const uint2 screen_size_int(oclraster::get_width(), oclraster::get_height());
		const double2 screen_size(screen_size_int);
		
		////////////////////////////////
		// linux/windows version
#if !defined(__APPLE__)
		SDL_GetMouseState(&cursor_pos_x, &cursor_pos_y);
		const double xpos = (1.0 / screen_size.x) * (double)cursor_pos_x;
		const double ypos = (1.0 / screen_size.y) * (double)cursor_pos_y;
		if(xpos != 0.5 || ypos != 0.5) {
			rotation.x -= (0.5 - ypos) * cam_rotation_speed;
			rotation.y -= (0.5 - xpos) * cam_rotation_speed;
			SDL_WarpMouseInWindow(oclraster::get_window(), screen_size_int.x/2, screen_size_int.y/2);
		}
		////////////////////////////////
		// os x version
#else
		SDL_GetRelativeMouseState(&cursor_pos_x, &cursor_pos_y);
		const double xpos = (1.0 / screen_size.x) * (double)-cursor_pos_x;
		const double ypos = (1.0 / screen_size.y) * (double)-cursor_pos_y;
		if(xpos != 0.0 || ypos != 0.0) {
			if(cam_ignore_next_rotation == 0) {
				cam_rotation.x += ypos * cam_rotation_speed;
				cam_rotation.y -= xpos * cam_rotation_speed;
			}
			else cam_ignore_next_rotation--;
			
			const double2 center_point(screen_size * 0.5);
			SDL_WarpMouseInWindow(oclraster::get_window(), round(center_point.x), round(center_point.y));
		}
#endif
		////////////////////////////////
		
		// wrap around 360°
		cam_rotation.x = core::wrap(cam_rotation.x, 360.0f);
		cam_rotation.y = core::wrap(cam_rotation.y, 360.0f);
		
		const float2 unit_vec_x(sinf(DEG2RAD(cam_rotation.x)), cosf(DEG2RAD(cam_rotation.x)));
		const float2 unit_vec_y(sinf(DEG2RAD(cam_rotation.y)), cosf(DEG2RAD(cam_rotation.y)));
		
		cam_forward.x = unit_vec_y.x;
		cam_forward.y = unit_vec_x.x;
		cam_forward.z = unit_vec_y.y;
		cam_forward.normalize();
		
		cam_up.x = 0.0f;
		cam_up.y = -unit_vec_x.y;
		cam_up.z = -unit_vec_x.x;
		cam_up = (cam_up ^ cam_forward).normalized() ^ cam_forward;
	}
	return true;
}

bool quit_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	done = true;
	return true;
}
