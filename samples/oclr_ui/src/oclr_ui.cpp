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

#include "oclr_ui.h"

// global vars, don't change these!
static bool done = false;
static event* evt = nullptr;
static camera* cam = nullptr;
static constexpr float3 cam_speeds { 0.01f, 0.1f, 0.001f };
static vector<transform_program*> transform_programs;
static vector<rasterization_program*> rasterization_programs;
static transform_program* ui_tp { nullptr };
static rasterization_program* ui_rp { nullptr };
static pipeline* p { nullptr };
static gui* ui { nullptr };
static font_manager* fm { nullptr };
static gui_theme* ui_theme { nullptr };
static gui_window* ui_wnd { nullptr };
static gui_button* ui_button { nullptr };

void primitive_test_draw(const DRAW_MODE_UI draw_mode, const framebuffer* buffer);


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
	
	//
	p = new pipeline();
	oclraster::set_active_pipeline(p);
	oclraster_support::init(p);
	ui = oclraster_support::get_gui();
	fm = ui->get_font_manager();
	ui_theme = ui->get_theme();
	
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
	oclraster_struct tp_uniforms {
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
	
	float light_pos = 0.0f, light_dist = 10.0f;
	oclraster_struct rp_uniforms {
		float4 camera_position;
		float4 light_position; // .w = light radius ^ 2
		float4 light_color;
	} rasterize_uniforms {
		float4(cam->get_position(), 1.0f),
		float4(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, 16.0f*16.0f),
		float4(0.0f, 0.3f, 0.7f, 1.0f)
	};
	opencl::buffer_object* rp_uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																   opencl::BUFFER_FLAG::INITIAL_COPY |
																   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																   sizeof(rp_uniforms),
																   (void*)&rasterize_uniforms);
	
	
	// init ui
	ui_wnd = ui->add<gui_window>(float2(1.0f), float2(0.0f));
	ui_button = ui->add<gui_button>(float2(0.1f, 0.03f), float2(0.1f));
	ui_button->set_label("test button");
	ui_wnd->add_child(ui_button);
	
	gui_text* ui_txt = ui->add<gui_text>(float2(0.1f, 0.03f), float2(0.0f, 0.1f));
	ui_txt->set_label("Test text:");
	gui_input_box* ui_input = ui->add<gui_input_box>(float2(0.1f, 0.03f), float2(0.0f, 0.2f));
	gui_pop_up_button* ui_popup = ui->add<gui_pop_up_button>(float2(0.1f, 0.03f), float2(0.0f, 0.3f));
	gui_pop_up_button* ui_popup2 = ui->add<gui_pop_up_button>(float2(0.18f, 0.03f), float2(0.0f, 0.35f));
	ui_wnd->add_child(ui_txt);
	ui_wnd->add_child(ui_input);
	ui_wnd->add_child(ui_popup);
	ui_wnd->add_child(ui_popup2);
	
	ui_popup->add_item("t1", "Test 1");
	ui_popup->add_item("t2", "test 2!");
	ui_popup->add_item("t3", "test 3");
	ui_popup->add_item("t4", "AO@€äÖÜ!_");
	
	ui_popup2->add_item("t1", "<i>Test 1</i>");
	ui_popup2->add_item("t2", "<b>test 2!</b>");
	ui_popup2->add_item("t3", "<i><b>#test 3<b></i>");
	ui_popup2->add_item("t4", "AO@€äÖÜ!_");
	
	ui_popup->set_selected_item(2);
	
	gui_slider* slider = ui->add<gui_slider>(float2(0.2f, 0.03f), float2(0.0f, 0.5f));
	ui_wnd->add_child(slider);
	
	gui_list_box* listbox = ui->add<gui_list_box>(float2(0.3f, 0.25f), float2(0.0f, 0.55f));
	listbox->add_item("t1", "Test 1");
	listbox->add_item("t2", "test 2!");
	listbox->add_item("t3", "test 3");
	listbox->add_item("t4", "AO@€äÖÜ!_");
	listbox->add_item("t21", "<i>test 1</i>");
	listbox->add_item("t22", "<b>test 2!</b>");
	listbox->add_item("t23", "<i><b>test 3</b></i>");
	listbox->add_item("multi", "こんにちわمرحبا你好שלוםनमस्तेприветΓεια σας.....................!");
	for(size_t i = 0; i < 8; i++) {
		listbox->add_item(size_t2string(i), "item #"+size_t2string(i));
	}
	ui_wnd->add_child(listbox);
	
	gui_window* local_wnd = ui_wnd;
	ui_button->add_handler([&local_wnd](GUI_EVENT, gui_object&) {
		cout << "button press!" << endl;
		oclraster::acquire_context();
		local_wnd->lock();
		local_wnd->set_position(float2(0.1f - ui_wnd->get_position().x, 0.0f));
		local_wnd->unlock();
		oclraster::release_context();
	}, GUI_EVENT::BUTTON_PRESS);
	
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
		cam->run();
		p->set_camera(cam);
		
		// update uniforms
		light_pos -= 0.25f;
		rasterize_uniforms.camera_position = cam->get_position();
		rasterize_uniforms.light_position.set(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, 16.0f*16.0f);
		static constexpr float color_step_range = 0.2f;
		rasterize_uniforms.light_color.x += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.y += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.z += core::rand(-color_step_range, color_step_range);
		rasterize_uniforms.light_color.clamp(0.0f, 1.0f);
		ocl->write_buffer(rp_uniforms_buffer, &rasterize_uniforms);
		
		// draw something
		p->bind_program(*ui_tp);
		p->bind_program(*ui_rp);
		p->bind_buffer("index_buffer", model->get_index_buffer(0));
		p->bind_buffer("input_attributes", model->get_vertex_buffer());
		p->bind_buffer("tp_uniforms", *tp_uniforms_buffer);
		p->bind_buffer("rp_uniforms", *rp_uniforms_buffer);
		p->draw(PRIMITIVE_TYPE::TRIANGLE, model->get_vertex_count(), { 0, model->get_index_count(0) });
		
		//
		//primitive_test_draw(DRAW_MODE_UI::POST_UI, p->get_default_framebuffer());
		ui->draw();
		
		oclraster::stop_draw();
	}
	
	// cleanup
	delete model;
	delete cam;
	
	ocl->delete_buffer(tp_uniforms_buffer);
	ocl->delete_buffer(rp_uniforms_buffer);
	
	evt->remove_event_handler(key_handler_fnctr);
	evt->remove_event_handler(mouse_handler_fnctr);
	evt->remove_event_handler(quit_handler_fnctr);
	delete p;
	oclraster_support::destroy();
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
		{{ "simple_shader_vs.cl", "simple_shader_fs.cl" }},
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
	ui_tp = transform_programs[0];
	ui_rp = rasterization_programs[0];
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
			case SDLK_F18:
			case SDLK_9:
				ui->get_theme()->reload();
				ui_wnd->redraw();
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

void primitive_test_draw(const DRAW_MODE_UI draw_mode oclr_unused, const framebuffer* buffer oclr_unused) {
	// enter 2d rendering
	p->start_orthographic_rendering();
	
	//gfx2d::draw_rectangle_fill(rect { 0, 0, 100, 100 }, float4(1.0f, 0.0f, 0.0f, 1.0f));
	//gfx2d::draw_rectangle_fill(float4 { 0.0f, 0.0f, 20.0f / (float)buffer->get_size().x, 20.0f / (float)buffer->get_size().y }, float4(1.0f, 0.0f, 0.0f, 1.0f));
	/*gfx2d::draw_rectangle_fill(float4(50.0f, 50.0f, 150.f, 150.0f), float4(1.0f, 0.0f, 0.0f, 1.0f));
	gfx2d::draw_rectangle_fill(float4(150.0f, 50.0f, 250.f, 150.0f), float4(0.0f, 0.0f, 1.0f, 1.0f));
	gfx2d::draw_rectangle_fill(float4(50.0f, 150.0f, 150.f, 250.0f), float4(0.0f, 1.0f, 0.0f, 1.0f));
	gfx2d::draw_rectangle_fill(float4(150.0f, 150.0f, 250.f, 250.0f), float4(1.0f, 0.0f, 1.0f, 1.0f));*/
	
	/*gfx2d::draw_rounded_rectangle_fill(float4(50.0f, 50.0f, 150.f, 150.0f), 16.0f, gfx2d::CORNER::ALL, float4(1.0f, 0.0f, 0.0f, 1.0f));
	gfx2d::draw_rounded_rectangle_fill(float4(150.0f, 50.0f, 250.f, 150.0f), 16.0f, gfx2d::CORNER::NONE, float4(0.0f, 0.0f, 1.0f, 1.0f));
	gfx2d::draw_rounded_rectangle_fill(float4(50.0f, 150.0f, 150.f, 250.0f), 16.0f, gfx2d::CORNER::TOP_LEFT | gfx2d::CORNER::BOTTOM_RIGHT,
									   float4(0.0f, 1.0f, 0.0f, 1.0f));
	gfx2d::draw_rounded_rectangle_border_gradient(float4(150.0f, 150.0f, 250.f, 250.0f), 16.0f, gfx2d::CORNER::ALL,
												  5.0f,
												  gfx2d::GRADIENT_TYPE::VERTICAL, float4 { 0.0f, 0.35f, 0.65f, 1.0f },
												  vector<float4> {
													  float4(1.0f, 0.0f, 1.0f, 1.0f),
													  float4(1.0f, 0.5f, 0.0f, 1.0f),
													  float4(0.0f, 1.0f, 1.0f, 1.0f),
													  float4(1.0f, 0.0f, 0.0f, 1.0f),
												  });
	gfx2d::draw_rounded_rectangle_border_fill(float4(250.0f, 50.0f, 350.f, 150.0f), 4.6666666f, gfx2d::CORNER::ALL,
											  1.0f,
											  float4(1.0f, 0.0f, 0.0f, 1.0f));*/
	
	//gfx2d::draw_circle_sector_fill(float2(350.0f, 50.0f), 32.0f, 32.0f, 270.0f, 360.0f, float4(1.0f, 1.0f, 0.0f, 1.0f));
	
	gfx2d::draw_rectangle_texture(float4(0, 0, 720, 720), *ui->get_font_manager()->get_font("SYSTEM_SANS_SERIF")->_get_image(),
								  float2(0.0f, 1.0f), float2(1.0f, 0.0f));
	
#if 0
	// real clock test
	const auto now = chrono::system_clock::now();
	const auto diff = now - chrono::system_clock::from_time_t(chrono::system_clock::to_time_t(now));
	const auto t = chrono::system_clock::to_time_t(now);
	const auto ltm = localtime(&t);
	
	const uint2 buffer_size(buffer->get_size());
	const float min_size = (float)std::min(buffer_size.x, buffer_size.y);
	const float clock_size = min_size * 0.75f;
	const float clock_radius = clock_size * 0.5f;
	const float2 clock_center(buffer_size / 2);
	
	gfx2d::draw_circle_border_fill(clock_center, clock_radius, clock_radius, 2.5f, float4(1.0f));
	gfx2d::draw_circle_border_fill(clock_center, 10.0f, 10.0f, 1.2f, float4(1.0f));
	// clock hands
	const float3 handle_sizes(10.0f, 10.0f, 2.5f);
	const float3 handle_length(clock_radius*0.6f, clock_radius*0.85f, clock_radius*0.98f);
	const float hour_angle = DEG2RAD(fmodf((360.0f * (float(ltm->tm_hour % 12) / 12.0f)), 360.0f));
	const float min_angle = DEG2RAD(fmodf((360.0f * (float(ltm->tm_min % 60) / 60.0f)), 360.0f));
	const float sec_angle = DEG2RAD(fmodf((360.0f * (float(ltm->tm_sec % 60) / 60.0f)), 360.0f));
	gfx2d::draw_line_fill(clock_center, clock_center + float2(sinf(hour_angle)*handle_length.x, cosf(hour_angle)*handle_length.x), handle_sizes.x, float4(1.0f));
	gfx2d::draw_line_fill(clock_center, clock_center + float2(sinf(min_angle)*handle_length.y, cosf(min_angle)*handle_length.y), handle_sizes.y, float4(1.0f));
	gfx2d::draw_line_fill(clock_center, clock_center + float2(sinf(sec_angle)*handle_length.z, cosf(sec_angle)*handle_length.z), handle_sizes.z, float4(1.0f));
	
	for(size_t i = 0; i < 12; i++) {
		const float angle = DEG2RAD((float(i) / 12.0f) * 360.0f);
		const float2 line_p(sinf(angle), cosf(angle));
		const float len = ((i % 3) == 0 ? 0.9f : 0.95f);
		const float thickness = ((i % 3) == 0 ? 2.5f : 1.5f);
		gfx2d::draw_line_fill(clock_center + line_p*(clock_radius*len), clock_center + line_p*clock_radius, thickness, float4(1.0f));
	}
	
	//
	if((ltm->tm_sec % 60) == 0) {
		const float dur = float(diff.count())*2.0f / float(decltype(diff)::period::den);
		gfx2d::draw_circle_fill(clock_center, clock_radius, clock_radius, float4(1.0f, 1.0f, 1.0f, (dur > 1.0f) ? 0.0f : (1.0f - dur)));
	}
#endif
	
	// exit 2d rendering
	p->stop_orthographic_rendering();
}
