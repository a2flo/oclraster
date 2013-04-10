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

#include "oclr_rtt.h"

// global vars, don't change these!
static bool done = false;
static event* evt = nullptr;
static camera* cam = nullptr;
static camera* monkey_cam = nullptr;
static constexpr float3 cam_speeds { 0.01f, 0.1f, 0.001f };
static atomic<unsigned int> update_model { true };
static vector<transform_program*> transform_programs;
static vector<rasterization_program*> rasterization_programs;
static transform_program* rtt_tp { nullptr };
static rasterization_program* rtt_rp { nullptr };
static transform_program* rtt_display_tp { nullptr };
static rasterization_program* rtt_display_rp { nullptr };
static pipeline* p { nullptr };
static constexpr size_t material_count { 1 };

int main(int argc oclr_unused, char* argv[]) {
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
	cam->set_position(0.8f, 0.28f, 3.2f);
	cam->set_rotation(-5.2f, 196.0f, 0.0f);
	cam->set_speed(cam_speeds.x);
	cam->set_rotation_speed(cam->get_rotation_speed() * 1.5f);
	cam->set_wasd_input(true);
	
	monkey_cam = new camera();
	monkey_cam->set_position(0.8f, 0.28f, 3.2f);
	monkey_cam->set_rotation(-5.2f, 196.0f, 0.0f);
	monkey_cam->set_speed(0.0f);
	monkey_cam->set_rotation_speed(0.0f);
	monkey_cam->set_wasd_input(false);
	monkey_cam->set_keyboard_input(false);
	monkey_cam->set_mouse_input(false);
	
	//
	p = new pipeline();
	
	a2m* model = new a2m(oclraster::data_path("monkey_uv.a2m"));
	
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
	
	// create / ref buffers
	const opencl::buffer_object& index_buffer = model->get_index_buffer(0);
	const opencl::buffer_object& input_attributes = model->get_vertex_buffer();
	
	struct __attribute__((packed, aligned(16))) tp_uniforms {
		matrix4f rotation;
		matrix4f modelview;
	} transform_uniforms {
		matrix4f(),
		matrix4f()
	};
	opencl::buffer_object* tp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(tp_uniforms),
																   (void*)&transform_uniforms);
	
	// textures
	static const array<string, material_count*3> texture_names {
		{
			"planks_512",
			"planks_normal_512",
			"planks_height_512",
		}
	};
	
	array<array<image, 3>, material_count> materials {{ // excessive braces are excessive
		{{
			image::from_file(oclraster::data_path(texture_names[0]+".png"), image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA),
			image::from_file(oclraster::data_path(texture_names[1]+".png"), image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA),
			image::from_file(oclraster::data_path(texture_names[2]+".png"), image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA)
		}}
	}};
	
	// plane model
	struct __attribute__((packed, aligned(16))) plane_vertex_attribute {
		float4 vertex;
		float2 tex_coord;
	};
	const array<plane_vertex_attribute, 4> plane_attributes {{
		{ float4 { -1.0f, -1.0f, 0.0f, 1.0f }, float2 { 0.0f, 0.0f } },
		{ float4 { -1.0f, 1.0f, 0.0f, 1.0f }, float2 { 0.0f, 1.0f } },
		{ float4 { 1.0f, 1.0f, 0.0f, 1.0f }, float2 { 1.0f, 1.0f } },
		{ float4 { 1.0f, -1.0f, 0.0f, 1.0f }, float2 { 1.0f, 0.0f } },
	}};
	opencl::buffer_object* plane_input_attributes = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																	   opencl::BUFFER_FLAG::INITIAL_COPY |
																	   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																	   sizeof(plane_vertex_attribute) * plane_attributes.size(),
																	   (void*)&plane_attributes[0]);
	
	const vector<index3> plane_indices {
		{ { 0, 1, 2 }, { 0, 2, 3 }, { 0, 2, 1 }, { 0, 3, 2 } }
	};
	opencl::buffer_object* plane_index_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(index3) * plane_indices.size(),
																   (void*)&plane_indices[0]);
	
	// rtt framebuffer
	const uint2 render_size(512, 512);
	framebuffer rtt_fb = framebuffer::create_with_images(render_size.x, render_size.y,
														 {{ IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA }},
														 { IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R });
	rtt_fb.set_clear_color(ulong4 { 0, 76, 180, 255 });
	
	// init done
	oclraster::release_context();
	
	// main loop
	float model_rotation = 0.0f;
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
		cam->run();
		monkey_cam->run();
		
		//
		rtt_fb.clear();
		p->bind_framebuffer(&rtt_fb);
		p->bind_program(*rtt_tp);
		p->bind_program(*rtt_rp);
		p->set_camera(monkey_cam);
		
		// update uniforms
		if(update_model) {
			transform_uniforms.modelview = matrix4f().rotate_y(model_rotation);
			transform_uniforms.rotation = transform_uniforms.modelview;
			model_rotation += 1.5f;
			if(model_rotation >= 360.0f) {
				model_rotation = core::wrap(model_rotation, 360.0f);
			}
			ocl->write_buffer(tp_uniforms_buffer, &transform_uniforms);
		}
		
		// draw monkey to rtt framebuffer
		p->bind_buffer("index_buffer", index_buffer);
		p->bind_buffer("input_attributes", input_attributes);
		p->bind_buffer("tp_uniforms", *tp_uniforms_buffer);
		p->bind_image("diffuse_texture", materials[0][0]);
		p->draw(PRIMITIVE_TYPE::TRIANGLE, model->get_vertex_count(), { 0, model->get_index_count(0) });
		
		//
		p->bind_framebuffer(nullptr);
		p->set_camera(cam);
		
		p->bind_program(*rtt_display_tp);
		p->bind_program(*rtt_display_rp);
		p->bind_buffer("index_buffer", *plane_index_buffer);
		p->bind_buffer("input_attributes", *plane_input_attributes);
		p->bind_image("texture", *rtt_fb.get_image(0));
		p->draw(PRIMITIVE_TYPE::TRIANGLE, plane_attributes.size(), { 0, plane_indices.size() });
		
		p->swap();
		oclraster::stop_draw();
	}
	
	// cleanup
	framebuffer::destroy_images(rtt_fb);
	delete model;
	delete monkey_cam;
	delete cam;
	
	ocl->delete_buffer(tp_uniforms_buffer);
	
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
		{{ "diffuse_texturing_vs.cl", "diffuse_texturing_fs.cl" }},
		{{ "rtt_display_vs.cl", "rtt_display_fs.cl" }},
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
	rtt_tp = transform_programs[0];
	rtt_rp = rasterization_programs[0];
	rtt_display_tp = transform_programs[1];
	rtt_display_rp = rasterization_programs[1];
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
