/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
 *  Copyright (C) 2004 - 2013 Florian Ziesche
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

#include "gui.h"
#include "rendering/shader.h"
#include "rendering/gfx2d.h"
#include "rendering/gl_timer.h"
#include "gui/font_manager.h"
#include "gui/style/gui_theme.h"
#include "gui/objects/gui_window.h"
#include "oclraster_support.h"

gui::gui(const string& theme_name) :
thread_base("gui"),
evt(oclraster::get_event()),
fm(new font_manager()),
theme(new gui_theme(fm)),
oclr_pipeline(oclraster_support::get_pipeline()),
main_fbo(0, 0),
key_handler_fnctr(this, &gui::key_handler),
mouse_handler_fnctr(this, &gui::mouse_handler),
window_handler_fnctr(this, &gui::window_handler) {
	// init clipboard (before adding event handlers)
	if(SDL_HasClipboardText()) clipboard_text = SDL_GetClipboardText();
	
	// create keyboard/mouse event handlers
	// note: the events will be deferred from the handlers
	// -> make the handlers internal, so events don't get deferred twice
	evt->add_internal_event_handler(key_handler_fnctr,
									EVENT_TYPE::KEY_DOWN,
									EVENT_TYPE::KEY_UP,
									EVENT_TYPE::UNICODE_INPUT,
									EVENT_TYPE::CLIPBOARD_UPDATE);
	evt->add_internal_event_handler(mouse_handler_fnctr,
									EVENT_TYPE::MOUSE_LEFT_DOWN,
									EVENT_TYPE::MOUSE_LEFT_UP,
									EVENT_TYPE::MOUSE_LEFT_CLICK,
									EVENT_TYPE::MOUSE_LEFT_DOUBLE_CLICK,
									EVENT_TYPE::MOUSE_LEFT_HOLD,
									
									EVENT_TYPE::MOUSE_RIGHT_DOWN,
									EVENT_TYPE::MOUSE_RIGHT_UP,
									EVENT_TYPE::MOUSE_RIGHT_CLICK,
									EVENT_TYPE::MOUSE_RIGHT_DOUBLE_CLICK,
									EVENT_TYPE::MOUSE_RIGHT_HOLD,
									
									EVENT_TYPE::MOUSE_MIDDLE_DOWN,
									EVENT_TYPE::MOUSE_MIDDLE_UP,
									EVENT_TYPE::MOUSE_MIDDLE_CLICK,
									EVENT_TYPE::MOUSE_MIDDLE_DOUBLE_CLICK,
									EVENT_TYPE::MOUSE_MIDDLE_HOLD,
									
									EVENT_TYPE::MOUSE_MOVE,
									
									EVENT_TYPE::MOUSE_WHEEL_UP,
									EVENT_TYPE::MOUSE_WHEEL_DOWN);
	
	array<unsigned int, 4> indices {{ 0, 1, 2, 3 }};
	fullscreen_indices = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
											opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
											opencl::BUFFER_FLAG::INITIAL_COPY,
											sizeof(unsigned int) * indices.size(),
											&indices[0]);
	
	fullscreen_vertices = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
											 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											 sizeof(float4) * 4);
	
	recreate_buffers(size2(oclraster::get_width(), oclraster::get_height()));
	
	// load theme
	theme->load("gui/"+theme_name+"/"+theme_name+".a2etheme");
	
	//
	evt->add_internal_event_handler(window_handler_fnctr, EVENT_TYPE::WINDOW_RESIZE);
	
	// start actual gui thread after everything has been initialized
	this->set_thread_delay(0);
	this->start();
}

gui::~gui() {
	oclr_debug("deleting gui object");
	
	set_thread_should_finish();
	
	evt->remove_event_handler(key_handler_fnctr);
	evt->remove_event_handler(mouse_handler_fnctr);
	evt->remove_event_handler(window_handler_fnctr);
	
	delete_buffers();
	
	finish();
	
	for(const auto& surface : cb_surfaces) {
		delete surface.second;
	}
	
	for(const auto& wnd : windows) {
		delete wnd;
	}
	
	delete theme;
	delete fm;
	
	if(fullscreen_vertices != nullptr) {
		ocl->delete_buffer(fullscreen_vertices);
	}
	if(fullscreen_indices != nullptr) {
		ocl->delete_buffer(fullscreen_indices);
	}

	oclr_debug("gui object deleted");
}

void gui::draw() {
	gl_timer::mark("GUI_START");
	
	const auto saved_depth_state = oclr_pipeline->get_depth_state();
	oclr_pipeline->set_depth_test(true);
	oclr_pipeline->set_depth_function(DEPTH_FUNCTION::LESS_OR_EQUAL);
	oclr_pipeline->set_scissor_test(true);
	oclr_pipeline->set_scissor_rectangle({ 0, 0 }, main_fbo.get_size());
	
	//////////////////////////////////////////////////////////////////
	// draw individual surfaces
	
	// draw surfaces that need a redraw
	for(const auto& cb_surface : cb_surfaces) {
		if(cb_surface.second->needs_redraw()) cb_surface.second->draw();
	}
	
	// draw windows
	// try_lock should prevent any dead-locking due to other threads having the gui lock
	// and also trying to get the engine lock; in addition, this should also lead to
	// smoother gui drawing (-> no halts due to event handling)
	if(try_lock()) {
		for(const auto& wnd : windows) {
			wnd->draw();
		}
		unlock();
	}
	
	//////////////////////////////////////////////////////////////////
	// blit all surfaces onto gui buffer
	oclr_pipeline->bind_framebuffer(&main_fbo);
	main_fbo.clear();
	oclr_pipeline->start_orthographic_rendering();
	
	shader_helper::texture_shd->use();
	
	texture_shd_uniforms uniforms {
		float4(0.0f),
		float4(0.0f, 0.0f, 1.0f, 1.0f), // orientation
		1.0f
	};
	
	// pre ui callbacks
	for(auto& cb : draw_callbacks[0]) {
		uniforms.extent = cb_surfaces[cb]->get_extent();
		shader_helper::texture_shd->set_uniforms(uniforms);
		cb_surfaces[cb]->blit();
	}
	
	// blit windows (in reverse order)
	// TODO: lock?
	for(auto riter = windows.crbegin(); riter != windows.crend(); riter++) {
		if((*riter)->is_visible()) {
			uniforms.extent = (*riter)->get_extent();
			shader_helper::texture_shd->set_uniforms(uniforms);
			(*riter)->blit();
		}
	}
	
	// post ui callbacks
	for(auto& cb : draw_callbacks[1]) {
		uniforms.extent = cb_surfaces[cb]->get_extent();
		shader_helper::texture_shd->set_uniforms(uniforms);
		cb_surfaces[cb]->blit();
	}
	
	// stop
	oclr_pipeline->bind_framebuffer(nullptr);
	
	// reset state
	oclr_pipeline->set_scissor_test(false);
	oclr_pipeline->set_depth_state(saved_depth_state);
	
	//////////////////////////////////////////////////////////////////
	// blend with scene buffer and draw result to the main framebuffer
	
	shader_helper::blend_shd->use("#", false, nullptr);
	oclr_pipeline->bind_image("src_buffer", *main_fbo.get_image(0));
	oclr_pipeline->bind_buffer("index_buffer", *fullscreen_indices);
	oclr_pipeline->bind_buffer("input_attributes", *fullscreen_vertices);
	
	oclr_pipeline->draw(PRIMITIVE_TYPE::TRIANGLE_STRIP, 4, { 0, 2 });
	
	oclr_pipeline->stop_orthographic_rendering();
	gl_timer::mark("GUI_END");
}

void gui::run() {
	if(!events_in_queue) return;
	
	// copy/swap events from the event queue to the processing queue
	event_lock.lock();
	event_processing_queue.swap(event_queue);
	events_in_queue = false;
	event_lock.unlock();
	
	// lock gui
	lock();
	
	//
	if(windows.empty()) {
		unlock();
		event_processing_queue.clear();
		return;
	}
	
	// lock all windows
	for(const auto& wnd : windows) {
		wnd->lock();
	}
	
	// handle events
	for(const auto& gevt : event_processing_queue) {
		// mouse events:
		if((gevt.first & EVENT_TYPE::__MOUSE_EVENT) == EVENT_TYPE::__MOUSE_EVENT) {
			// note: mouse down/up events only affect the draw state, mouse clicks do the actual
			// logic checking (e.g. a button was pressed), since they contain both positions
			ipnt check_point(-1, -1);
			switch(gevt.first) {
				case EVENT_TYPE::MOUSE_LEFT_DOWN:
				case EVENT_TYPE::MOUSE_LEFT_UP:
				case EVENT_TYPE::MOUSE_LEFT_HOLD:
				case EVENT_TYPE::MOUSE_RIGHT_DOWN:
				case EVENT_TYPE::MOUSE_RIGHT_UP:
				case EVENT_TYPE::MOUSE_RIGHT_HOLD:
				case EVENT_TYPE::MOUSE_MIDDLE_DOWN:
				case EVENT_TYPE::MOUSE_MIDDLE_UP:
				case EVENT_TYPE::MOUSE_MIDDLE_HOLD:
				case EVENT_TYPE::MOUSE_MOVE:
					check_point = ((const mouse_event_base<EVENT_TYPE::__MOUSE_EVENT>&)*gevt.second).position;
					break;
					
				case EVENT_TYPE::MOUSE_LEFT_CLICK:
				case EVENT_TYPE::MOUSE_LEFT_DOUBLE_CLICK:
				case EVENT_TYPE::MOUSE_RIGHT_CLICK:
				case EVENT_TYPE::MOUSE_RIGHT_DOUBLE_CLICK:
				case EVENT_TYPE::MOUSE_MIDDLE_CLICK:
				case EVENT_TYPE::MOUSE_MIDDLE_DOUBLE_CLICK:
					// check down, because up and down must both match (be within) in the target object
					// and the object will have already received a mouse down event
					// note: memory layout is the same for all click events (-> mouse_left_click_event)
					check_point = ((const mouse_left_click_event&)*gevt.second).down->position;
					break;
					
				case EVENT_TYPE::MOUSE_WHEEL_UP:
				case EVENT_TYPE::MOUSE_WHEEL_DOWN:
					// mouse wheel events actually also have a position and should only be sent to the
					// windows underneath that position (-> scrolling in inactive windows)
					check_point = ((const mouse_wheel_event_base<EVENT_TYPE::__MOUSE_EVENT>&)*gevt.second).position;
					break;
				default: break;
			}
			
			// check all windows
			bool handled = false;
			for(const auto& wnd : windows) {
				if(wnd->handle_mouse_event(gevt.first, gevt.second, check_point)) {
					handled = true;
					break;
				}
			}
			
			// if no gui object handled the event, do some additional handling:
			if(!handled) {
				//
				switch(gevt.first) {
					case EVENT_TYPE::MOUSE_LEFT_DOWN:
						// if no object handled the mouse down/click event,
						// no object can be active -> deactivate active object
						set_active_object(nullptr);
						break;
					default: break;
				}
			}
		}
		// key events:
		else if((gevt.first & EVENT_TYPE::__KEY_EVENT) == EVENT_TYPE::__KEY_EVENT) {
			// TODO: key events should only be sent to the active window
			/*if(!windows.empty()) {
				windows[0]->handle_key_event(gevt.first, gevt.second);
			}*/
			// for now: send to all windows
			for(const auto& wnd : windows) {
				if(wnd->handle_key_event(gevt.first, gevt.second)) {
					break;
				}
			}
		}
	}
	
	// unlock again
	for(const auto& wnd : windows) {
		wnd->unlock();
	}
	unlock();
	
	event_processing_queue.clear(); // *done*
}

bool gui::key_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::CLIPBOARD_UPDATE) {
		clipboard_text = ((const clipboard_update_event&)*obj).text;
		return true;
	}
	
	if(!keyboard_input) return false;
	event_lock.lock();
	event_queue.emplace_back(type, obj);
	events_in_queue = true;
	event_lock.unlock();
	return true;
}

bool gui::mouse_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(!mouse_input) return false;
	event_lock.lock();
	event_queue.emplace_back(type, obj);
	events_in_queue = true;
	event_lock.unlock();
	return true;
}

bool gui::window_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::WINDOW_RESIZE) {
		const window_resize_event& evtobj = (const window_resize_event&)*obj;
		recreate_buffers(evtobj.size);
	}
	return true;
}

void gui::set_keyboard_input(const bool& state) {
	keyboard_input = state;
}

void gui::set_mouse_input(const bool& state) {
	mouse_input = state;
}

bool gui::get_keyboard_input() const {
	return keyboard_input;
}

bool gui::get_mouse_input() const {
	return mouse_input;
}

gui_simple_callback* gui::add_draw_callback(const DRAW_MODE_UI& mode, ui_draw_callback& cb,
											const float2& size, const float2& offset,
											const gui_surface::SURFACE_FLAGS flags) {
	auto& callbacks = draw_callbacks[mode == DRAW_MODE_UI::PRE_UI ? 0 : 1];
	const auto iter = find(begin(callbacks), end(callbacks), &cb);
	if(iter != end(callbacks)) {
		oclr_error("this ui draw callback already exists!");
		return nullptr;
	}
	callbacks.emplace_back(&cb);
	
	const auto surface_iter = cb_surfaces.find(&cb);
	if(surface_iter != cb_surfaces.end()) return surface_iter->second;
	
	gui_simple_callback* surface = new gui_simple_callback(cb, mode, size, offset, flags);
	cb_surfaces.insert(make_pair(&cb, surface));
	return surface;
}

void gui::delete_draw_callback(ui_draw_callback& cb) {
	const auto iter_0 = find(begin(draw_callbacks[0]), end(draw_callbacks[0]), &cb);
	const auto iter_1 = find(begin(draw_callbacks[1]), end(draw_callbacks[1]), &cb);
	
	if(iter_0 == end(draw_callbacks[0]) && iter_1 == end(draw_callbacks[1])) {
		oclr_error("no such ui draw callback does exist!");
		return;
	}
	
	if(iter_0 != end(draw_callbacks[0])) draw_callbacks[0].erase(iter_0);
	if(iter_1 != end(draw_callbacks[1])) draw_callbacks[1].erase(iter_1);
	
	const auto surface_iter = cb_surfaces.find(&cb);
	if(surface_iter != cb_surfaces.end()) {
		delete surface_iter->second;
		cb_surfaces.erase(surface_iter);
	}
}

void gui::recreate_buffers(const size2 size) {
	delete_buffers();
	
	// create main gui buffer
	main_fbo = framebuffer::create_with_images((unsigned int)size.x, (unsigned int)size.y,
											   { { IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA } },
											   { IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R });
	
	// resize/recreate surfaces
	for(const auto& surface : cb_surfaces) {
		surface.second->resize(surface.second->get_buffer_size());
	}
	
	// resize/recreate window surfaces
	for(const auto& wnd : windows) {
		wnd->resize(wnd->get_buffer_size());
	}
	
	//
	const auto default_fb_size = oclr_pipeline->get_default_framebuffer()->get_size();
	array<float4, 4> vertices {{
		float4(0.0f, (float)default_fb_size.y, 0.0f, 1.0f),
		float4(0.0f, 0.0f, 0.0f, 1.0f),
		float4((float)default_fb_size.x, (float)default_fb_size.y, 0.0f, 1.0f),
		float4((float)default_fb_size.x, 0.0f, 0.0f, 1.0f),
	}};
	ocl->write_buffer(fullscreen_vertices, &vertices[0]);
}

void gui::delete_buffers() {
	framebuffer::destroy_images(main_fbo);
}

font_manager* gui::get_font_manager() const {
	return fm;
}

gui_theme* gui::get_theme() const {
	return theme;
}

void gui::add_window(gui_window* wnd) {
	windows.emplace_front(wnd);
}

void gui::remove_window(gui_window* wnd) {
	const auto iter = find(begin(windows), end(windows), wnd);
	if(iter != end(windows)) {
		windows.erase(iter);
	}
}

void gui::lock() {
	object_lock.lock();
}

bool gui::try_lock() {
	return object_lock.try_lock();
}

void gui::unlock() {
	object_lock.unlock();
}

bool gui::set_clipboard_text(const string& text) {
	if(SDL_SetClipboardText(text.c_str()) != 0) {
		oclr_error("couldn't set clipboard text: %s!", SDL_GetError());
		return false;
	}
	clipboard_text = text;
	return true;
}

const string& gui::get_clipboard_text() const {
	return clipboard_text;
}

void gui::set_active_object(gui_object* obj) {
	gui_object* old_active_object = active_object;
	active_object = obj;
	
	// deactivate old object
	if(old_active_object != nullptr) {
		old_active_object->set_active(false);
	}
	
	// activate new object
	if(active_object != nullptr) {
		active_object->set_active(true);
	}
}

gui_object* gui::get_active_object() const {
	return active_object;
}
