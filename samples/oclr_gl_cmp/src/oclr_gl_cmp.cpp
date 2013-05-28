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

#include "oclr_gl_cmp.h"
#include "gl_renderer.h"

// global vars, don't change these!
static bool done { false };
static event* evt { nullptr };
static camera* cam { nullptr };
static constexpr float3 cam_speeds { 0.01f, 0.1f, 0.001f };
static atomic<unsigned int> update_model { false };
static atomic<unsigned int> update_light { false };
static atomic<unsigned int> render_gl { true };
static transform_program* transform_prog { nullptr };
static rasterization_program* rasterization_prog { nullptr };
static pipeline* p { nullptr };
static atomic<unsigned int> selected_material { 0 };
static constexpr size_t material_count { 5 };

int main(int argc oclr_unused, char* argv[]) {
	// initialize oclraster
	oclraster::init(argv[0], (const char*)"../data/");
	oclraster::set_caption(APPLICATION_NAME);
	oclraster::acquire_context();
	
	// init class pointers
	evt = oclraster::get_event();
	//ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_CPU);
	ocl->set_active_device(opencl_base::DEVICE_TYPE::FASTEST_GPU);
	//ocl->set_active_device(opencl_base::DEVICE_TYPE::GPU0);
	
	//
	cam = new camera();
	//cam->set_position(0.0f, 0.0f, 0.0f);
	//cam->set_rotation(0.0f, 0.0f, 0.0f);
	cam->set_position(0.0f, -0.25f, 0.0f);
	cam->set_rotation(0.0f, 0.0f, 0.0f);
	cam->set_speed(cam_speeds.x);
	cam->set_rotation_speed(cam->get_rotation_speed() * 1.5f);
	cam->set_wasd_input(true);
	
	// create the pipeline, set the active camera and
	// notify oclraster that this is the active pipeline
	p = new pipeline();
	p->set_camera(cam);
	oclraster::set_active_pipeline(p);
	
	//
	init_gl_renderer();
	
	// create the model
	oclraster_struct vertex_data {
		float4 vertex;
		//float4 normal;
		//float2 tex_coord;
	};
	
	static constexpr size_t triangle_count = 32768;
	static constexpr size_t columns = 192;
	static constexpr float scale = 1.0f / (float)columns;
	static constexpr size_t index_count = triangle_count * 3;
	static constexpr size_t vertex_attr_count = triangle_count * 3;
	
	unsigned int* indices = new unsigned int[index_count];
	vertex_data* vertex_attrs = new vertex_data[vertex_attr_count];
	core::set_random_seed(42);
	//const auto& forward = p->get_camera_setup().forward;
	//const float4 offset { cam->get_position(), 1.0f };
	constexpr float2 offset { -0.5f };
	for(size_t i = 0; i < triangle_count; i++) {
		const size_t idx_offset = i * 3;
		indices[idx_offset + 0] = (unsigned int)(idx_offset + 0);
		indices[idx_offset + 1] = (unsigned int)(idx_offset + 1);
		indices[idx_offset + 2] = (unsigned int)(idx_offset + 2);
		
		auto& v0 = vertex_attrs[idx_offset + 0].vertex;
		auto& v1 = vertex_attrs[idx_offset + 1].vertex;
		auto& v2 = vertex_attrs[idx_offset + 2].vertex;
		const auto column = i % columns;
		const auto row = i / columns;
		const float fcolumn = (float)column;
		const float frow = (float)row;
		for(size_t j = 0; j < 3; j++) {
			/*vertex_attrs[idx_offset + j].vertex = offset + float3 {
				forward * core::rand(3.5f, 5.0f) +
				float3 { core::rand(0.0f, 1.0f), core::rand(0.0f, 1.0f), core::rand(0.0f, 1.0f) } * 2.0f - 1.0f
			};*/
			vertex_attrs[idx_offset + j].vertex = {
				offset.x + (fcolumn + (j == 1 ? 1.0f : 0.0f)) * scale,
				offset.y + (frow + (j == 2 ? 1.0f : 0.0f)) * scale,
				0.75f,
				(float)j
			};
			//vertex_attrs[idx_offset + j].tex_coord.set(j < 2 ? 0.0f : 1.0f, j == 0 ? 0.0f : 1.0f);
		}
		
		// compute the normal + check if the triangle faces the camera (if not, flip it)
		float3 normal { ((v1 - v0) ^ (v2 - v0)).normalized() };
		if((v0).normalized().dot(normal) >= 0.0f) {
			const auto tmp = vertex_attrs[idx_offset + 1];
			vertex_attrs[idx_offset + 1] = vertex_attrs[idx_offset + 2];
			vertex_attrs[idx_offset + 2] = tmp;
			normal *= -1.0f;
		}
		/*vertex_attrs[idx_offset + 0].normal = normal;
		vertex_attrs[idx_offset + 1].normal = normal;
		vertex_attrs[idx_offset + 2].normal = normal;*/
	}
	
	opencl::buffer_object* index_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
															 opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
															 opencl::BUFFER_FLAG::INITIAL_COPY,
															 sizeof(unsigned int) * index_count,
															 indices);
	opencl::buffer_object* vertex_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
															  opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
															  opencl::BUFFER_FLAG::INITIAL_COPY,
															  sizeof(vertex_data) * vertex_attr_count,
															  vertex_attrs);
	
	create_gl_buffers(sizeof(unsigned int) * index_count, indices,
					  sizeof(vertex_data) * vertex_attr_count, vertex_attrs);
	
	// add event handlers
	event::handler key_handler_fnctr(&key_handler);
	evt->add_event_handler(key_handler_fnctr, EVENT_TYPE::KEY_DOWN, EVENT_TYPE::KEY_UP);
	event::handler mouse_handler_fnctr(&mouse_handler);
	evt->add_internal_event_handler(mouse_handler_fnctr, EVENT_TYPE::MOUSE_RIGHT_CLICK, EVENT_TYPE::MOUSE_MOVE);
	event::handler quit_handler_fnctr(&quit_handler);
	evt->add_event_handler(quit_handler_fnctr, EVENT_TYPE::QUIT);
	event::handler kernel_reload_handler_fnctr(&kernel_reload_handler);
	evt->add_internal_event_handler(kernel_reload_handler_fnctr, EVENT_TYPE::KERNEL_RELOAD);
	
	// load, compile and bind user shaders
	if(!load_programs()) return -1;
	
	// create / ref buffers
	oclraster_struct tp_uniforms {
		matrix4f modelview;
	} transform_uniforms {
		matrix4f()
	};
	opencl::buffer_object* tp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(tp_uniforms),
																   (void*)&transform_uniforms);
	
	float light_pos = PI / 2.0f, light_dist = 10.0f, light_intensity = 32.0f;
	oclraster_struct rp_uniforms {
		float4 camera_position;
		float4 light_position; // .w = light radius ^ 2
		float4 light_color;
	} rasterize_uniforms {
		float4(cam->get_position(), 1.0f),
		float4(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, light_intensity*light_intensity),
		float4(0.0f, 0.3f, 0.7f, 1.0f)
	};
	opencl::buffer_object* rp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(rp_uniforms),
																   (void*)&rasterize_uniforms);
	
	// textures
	static constexpr size_t textures_per_material = 1;
	static const array<string, material_count * textures_per_material> texture_names {
		{
			"light_512",
			"planks_512",
			"rockwall_512",
			"acid_512",
			"blend_test_512",
		}
	};
	
	array<array<shared_ptr<image>, textures_per_material>, material_count> materials;
	for(size_t i = 0; i < material_count; i++) {
		for(size_t j = 0; j < textures_per_material; j++) {
			materials[i][j] = make_shared<image>(image::from_file(oclraster::data_path(texture_names[(i * textures_per_material) + j]+".png"),
																  image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA));
		}
	}
	
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
		
		//cout << endl << endl << "################" << endl << endl;
		
		oclraster::start_draw();
		cam->run();
		
		if(!render_gl) {
			oclraster::set_active_pipeline(p);
			p->set_camera(cam); // update pipeline camera
			
			// update uniforms
			if(update_model) {
				transform_uniforms.modelview = matrix4f().rotate_y(model_rotation);
				model_rotation += 1.0f;
				if(model_rotation >= 360.0f) {
					selected_material = (selected_material + 1) % material_count;
					model_rotation = core::wrap(model_rotation, 360.0f);
				}
				ocl->write_buffer(tp_uniforms_buffer, &transform_uniforms);
			}
			
			if(update_light) {
				light_pos -= 0.125f;
				rasterize_uniforms.light_position.set(sinf(light_pos)*light_dist,
													  0.0f,
													  cosf(light_pos)*light_dist,
													  light_intensity * light_intensity);
			}
			
			rasterize_uniforms.camera_position.vector3<float>::set(cam->get_position());
			ocl->write_buffer(rp_uniforms_buffer, &rasterize_uniforms);
			
			// draw something
			p->bind_buffer("index_buffer", *index_buffer);
			p->bind_buffer("input_attributes", *vertex_buffer);
			p->bind_buffer("tp_uniforms", *tp_uniforms_buffer);
			p->bind_buffer("rp_uniforms", *rp_uniforms_buffer);
			p->bind_image("diffuse_texture", *materials[selected_material][0]);
			p->draw(PRIMITIVE_TYPE::TRIANGLE, vertex_attr_count, { 0, triangle_count });
		}
		else {
			oclraster::set_active_pipeline(nullptr);
			gl_render(cam);
		}
		
		oclraster::stop_draw();
	}
	
	// cleanup
	destroy_gl_renderer();
	
	for(auto& mat : materials) {
		for(auto& tex : mat) {
			tex = nullptr; // clear
		}
	}
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

bool load_programs() {
	if(transform_prog != nullptr) {
		delete transform_prog;
		transform_prog = nullptr;
	}
	if(rasterization_prog != nullptr) {
		delete rasterization_prog;
		rasterization_prog = nullptr;
	}
	
	string vs_str, fs_str;
	static const array<string, 2> shader_filenames {
		{ "gl_cmp.cl", "gl_cmp.cl" }
	};
	
	if(!file_io::file_to_string(oclraster::kernel_path("user/"+shader_filenames[0]), vs_str)) {
		oclr_error("couldn't open vs program!");
		return false;
	}
	if(!file_io::file_to_string(oclraster::kernel_path("user/"+shader_filenames[1]), fs_str)) {
		oclr_error("couldn't open fs program!");
		return false;
	}
	transform_prog = new transform_program(vs_str, "transform_main");
	rasterization_prog = new rasterization_program(fs_str, "rasterize_main");
	p->bind_program(*transform_prog);
	p->bind_program(*rasterization_prog);
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
			case SDLK_l:
				update_light ^= true;
				break;
			case SDLK_g:
				render_gl ^= true;
				break;
			case SDLK_1:
				selected_material = 0;
				break;
			case SDLK_2:
				selected_material = 1;
				break;
			case SDLK_3:
				selected_material = 2;
				break;
			case SDLK_4:
				selected_material = 3;
				break;
			case SDLK_5:
				selected_material = 4;
				break;
			default: return false;
		}
	}
	return true;
}

bool mouse_handler(EVENT_TYPE type, shared_ptr<event_object> obj oclr_unused) {
	if(type == EVENT_TYPE::MOUSE_RIGHT_CLICK) {
		cam->set_mouse_input(cam->get_mouse_input() ^ true);
	}
	return true;
}

bool quit_handler(EVENT_TYPE type oclr_unused, shared_ptr<event_object> obj oclr_unused) {
	done = true;
	return true;
}
