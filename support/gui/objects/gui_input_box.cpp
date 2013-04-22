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

#include "gui_input_box.h"
#include "font.h"
#include "gui.h"
#include "threading/task.h"
#include "oclraster_support.h"

gui_input_box::gui_input_box(const float2& size_, const float2& position_) :
gui_object(size_, position_) {
	input_text_margin.set(gui_theme::point_to_pixel(input_text_margin.x),
						  gui_theme::point_to_pixel(input_text_margin.y));
	compute_abs_values(); // since we override this, call it again
	update_text_display();
}

gui_input_box::~gui_input_box() {
	// make sure to kill the blink task if it's still active
	blink_task = false;
	while(!blink_task_end.load()) this_thread::yield();
}

void gui_input_box::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	
	// below the text
	theme->draw("input_box", state.active ? "active" : "normal",
				position_abs, size_abs);
	
	// draw the text
	// note: scissor will be reset in the next theme draw
	oclraster_support::get_pipeline()->set_scissor_rectangle(floorf(position_abs.x + input_text_margin.x),
															 floorf(position_abs.y + input_text_margin.y),
															 ceilf(input_size_abs.x), ceilf(input_size_abs.y));
	fnt->draw_cached(input, input_text_position, theme->get_color_scheme().get("TEXT_INVERSE"));
	
	// draw blink character
	if(blink_state && blink_task && state.active) {
		auto limited_size = size_abs;
		auto blink_position = position_abs;
		const float blink_offset = cursor_position - input_text_interval.x;
		blink_position.x += blink_offset;
		limited_size.x -= blink_offset;
		theme->draw("input_box", "blink",
					blink_position, limited_size,
					false); // don't clear underlying input box
	}
	
	// above the text
	theme->draw("input_box", state.active ? "active_top" : "normal_top",
				position_abs, size_abs,
				false); // don't clear underlying input box
}

void gui_input_box::set_active(const bool& active_state) {
	if(active_state == state.active) return;
	gui_object::set_active(active_state);
	if(state.active) {
		handle(GUI_EVENT::INPUT_BOX_ACTIVATION);
		
		// spawn blink task
		if(!blink_task) {
			// if a previous deactivate signalled to end the blink task, but it
			// hasn't ended yet, wait for it, before spawning a new task
			while(!blink_task_end.load()) this_thread::yield();
			
			blink_task = true;
			blink_task_end = false;
			task::spawn([this]() {
				//
				static constexpr size_t blink_interval = 500;
				static constexpr size_t sleep_interval = 50;
				unsigned int blink_timer = SDL_GetTicks();
				do {
					if(!blink_task) break;
					const unsigned int cur_ticks = SDL_GetTicks();
					if((cur_ticks - blink_timer) > blink_interval) {
						blink_timer = cur_ticks;
						blink_state = blink_state ^ true;
						state.redraw = true;
						this_thread::sleep_for(chrono::milliseconds(sleep_interval));
					}
				} while(true);
				blink_state = false;
				blink_task_end = true;
			});
		}
	}
	// kill the blink task
	else {
		blink_task = false;
		handle(GUI_EVENT::INPUT_BOX_DEACTIVATION);
	}
	redraw();
}

bool gui_input_box::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj oclr_unused, const ipnt& point) {
	if(!state.visible || !state.enabled) return false;
	switch(type) {
		// left and right mouse button will have the same behavior for now
		case EVENT_TYPE::MOUSE_RIGHT_DOWN:
		case EVENT_TYPE::MOUSE_LEFT_DOWN: {
			// if this input box isn't active yet, make it so
			if(!state.active) {
				ui->set_active_object(this);
			}
			
			const float2 point_in_text = point - (position_abs + input_text_margin);
			const float position_in_text = input_text_interval.x + point_in_text.x;
			ssize_t position = 0;
			if(advance_map.size() > 1) { // non-empty
				input_cursor = -1; // set to the end; the below code will only find a position inside the text
				for(const auto& advance : advance_map) {
					if(position_in_text < advance.x) {
						// found the character
						// get the previous advance and determine the side on which the cursor should be placed
						const float2& prev_advance = advance_map[position-1];
						const size_t side = (position_in_text - prev_advance.x) < (prev_advance.y * 0.5f) ? 1 : 0;
						input_cursor = position - side;
						break;
					}
					position++;
				}
				// handle the end/last position correctly
				if(input_cursor >= 0 && (size_t)input_cursor == unicode_input.size()) {
					input_cursor = -1;
				}
				update_text_display();
			}
			return true;
		}
		
		default: break;
	}
	return false;
}

#define invalid_input_cursor_err() \
{ oclr_error("line #%u: invalid input_cursor \"%u\" (#input: %u)", __LINE__, input_cursor, unicode_input.size()); }

bool gui_input_box::handle_key_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj) {
	if(!state.visible || !state.enabled || !state.active) return false;
	switch(type) {
		//
		case EVENT_TYPE::UNICODE_INPUT: {
			const shared_ptr<unicode_input_event>& key_evt = (shared_ptr<unicode_input_event>&)obj;
			
			// check if we need to cache the key/character
			if(!fnt->is_cached(key_evt->key)) {
				oclraster::acquire_context();
				fnt->cache(key_evt->key, key_evt->key);
				oclraster::release_context();
			}
			
			if(input_cursor != -1) {
				if(input_cursor >= 0 && input_cursor < (ssize_t)unicode_input.size()) {
					unicode_input.emplace(begin(unicode_input) + input_cursor, key_evt->key);
				}
				else invalid_input_cursor_err()
				input_cursor++;
			}
			else unicode_input.emplace_back(key_evt->key);
			
			update_input();
			update_text_display();
			handle(GUI_EVENT::INPUT_BOX_INPUT);
			return true;
		}
		
		//
		case EVENT_TYPE::KEY_DOWN: {
			const shared_ptr<key_down_event>& key_evt = (shared_ptr<key_down_event>&)obj;
			const SDL_Keymod mod = SDL_GetModState();
			bool handled = true;
			switch(key_evt->key) {
				case SDLK_BACKSPACE:
					if(unicode_input.empty()) break;
					
					if(input_cursor == -1) {
						unicode_input.pop_back();
					}
					else if(input_cursor > 0) {
						input_cursor--;
						if(input_cursor >= 0 &&
						   input_cursor < (ssize_t)unicode_input.size()) {
							unicode_input.erase(begin(unicode_input) + input_cursor);
						}
						else invalid_input_cursor_err()
					}
					else break; // nothing to do here
					// update:
					update_input();
					update_text_display();
					break;
				case SDLK_DELETE:
					if(unicode_input.empty()) break;
					
					if(input_cursor != -1) {
						if(input_cursor >= 0 &&
						   input_cursor < (ssize_t)unicode_input.size()) {
							unicode_input.erase(begin(unicode_input) + input_cursor);
							update_input();
						}
						else invalid_input_cursor_err()
						
						if(input_cursor >= (ssize_t)unicode_input.size()) {
							input_cursor = -1;
						}
						update_text_display();
					}
					// else: nothing to do here
					break;
				case SDLK_LEFT:
					if(input_cursor != 0) {
						if(input_cursor == -1) {
							input_cursor = unicode_input.size();
						}
						input_cursor--;
						update_text_display();
					}
					break;
				case SDLK_RIGHT:
					if(input_cursor != -1) {
						input_cursor++;
						if(input_cursor >= (ssize_t)unicode_input.size()) {
							input_cursor = -1;
						}
						update_text_display();
					}
					break;
				// move cursor to end
				case SDLK_e:
					if((mod & (KMOD_RCTRL | KMOD_LCTRL)) == 0) {
						break;
					}
#if defined(__clang__)
					[[clang::fallthrough]];
#endif
				case SDLK_DOWN:
				case SDLK_END:
					input_cursor = -1;
					update_text_display();
					break;
				// move cursor to start
				case SDLK_a:
					if((mod & (KMOD_RCTRL | KMOD_LCTRL)) == 0) {
						break;
					}
#if defined(__clang__)
					[[clang::fallthrough]];
#endif
				case SDLK_UP:
				case SDLK_HOME:
					input_cursor = 0;
					update_text_display();
					break;
				case SDLK_k:
					// clear input
					if((mod & (KMOD_RCTRL | KMOD_LCTRL)) != 0) {
						unicode_input.clear();
						input_cursor = -1;
						update_input();
						update_text_display();
					}
					break;
				case SDLK_v: {
					// windows/linux/bsd/...: use ctrl for paste
					// os x: use cmd for paste
					if(
#if !defined(__APPLE__)
					   (mod & (KMOD_RCTRL | KMOD_LCTRL)) == 0
#else
					   (mod & (KMOD_RGUI | KMOD_LGUI)) == 0
#endif
					   ) {
						break;
					}
					const string& clipboard(ui->get_clipboard_text());
					if(clipboard.empty()) break;
					const vector<unsigned int> unicode_clipboard(unicode::utf8_to_unicode(clipboard));
					unicode_input.insert(input_cursor == -1 ?
										 end(unicode_input) : begin(unicode_input) + input_cursor,
										 begin(unicode_clipboard), end(unicode_clipboard));
					if(input_cursor != -1) input_cursor += unicode_clipboard.size();
					update_input();
					update_text_display();
				}
				break;
				case SDLK_RETURN:
				case SDLK_RETURN2:
				case SDLK_KP_ENTER:
					handle(GUI_EVENT::INPUT_BOX_ENTER);
					break;
				default:
					handled = false;
					break;
			}
			return handled;
		}
		default: break;
	}
	return false;
}

void gui_input_box::update_input() {
	input = unicode::unicode_to_utf8(unicode_input);
	advance_map = fnt->compute_advance_map(unicode_input);
	blink_state = true; // always show blink character on text input
}

void gui_input_box::update_text_display() {
	// compute the text position, displayed text interval and cursor position
	const float text_width = advance_map.back().x; // last element contains the total width
	
	input_text_position = position_abs;
	cursor_position = (input_cursor != -1 ? advance_map[input_cursor].x : text_width);
	if(text_width > input_size_abs.x) {
		// only a portition of the text fits into the box -> determine which
		if(input_cursor == -1) {
			// -> completely to the right
			input_text_interval.set(text_width - input_size_abs.x, text_width);
		}
		else {
			// -> somewhere in between
			if(cursor_position < input_text_interval.x || // cursor moved outside the interval
			   cursor_position > input_text_interval.y ||
			   text_width < input_text_interval.y) { // text was removed and is now smaller than the interval
				// new cursor position should be in the center (50/50 text split)
				const float half_size = input_size_abs.x * 0.5f;
				input_text_interval.set(cursor_position - half_size, cursor_position + half_size);
				if(input_text_interval.x < 0.0f) {
					// if we're on the left side, set the correct interval
					input_text_interval.set(0.0f, input_size_abs.x);
				}
				else if(input_text_interval.y >= text_width) {
					// if we're on the right side, set the correct interval
					input_text_interval.set(text_width - input_size_abs.x, text_width);
				}
			}
		}
		input_text_position.x -= input_text_interval.x;
		// for correct font rendering, floor position
		input_text_position.x = floorf(input_text_position.x);
	}
	else {
		// text completely fits into the input box -> just draw it
		input_text_interval.set(0.0f, text_width);
	}
	input_text_position.x += input_text_margin.x;
	input_text_position.y -= input_text_margin.y;
	input_text_position.floor(); // necessary for correct text rendering
	
	// something changed -> display blink character for a tick
	blink_state = true;
}

void gui_input_box::set_input(const string& input_) {
	input = input_;
	unicode_input.clear();
	unicode_input = unicode::utf8_to_unicode(input);
	advance_map = fnt->compute_advance_map(unicode_input);
	input_cursor = -1;
	update_text_display();
	redraw();
}

const string& gui_input_box::get_input() const {
	return input;
}

void gui_input_box::compute_abs_values() {
	gui_object::compute_abs_values();
	input_size_abs = float2(size_abs - float2(input_text_margin.x + input_text_margin.x,
											  input_text_margin.y + input_text_margin.y));
	update_text_display();
}
