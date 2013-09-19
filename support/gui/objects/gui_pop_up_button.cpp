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

#include "gui_pop_up_button.hpp"
#include "gui_window.hpp"
#include "core/event.hpp"
#include "gui.hpp"
#include "threading/task.hpp"
#include "font.hpp"
#include "oclraster_support.hpp"

////
class gui_pop_up_window : public gui_window {
public:
	gui_pop_up_window(const float2& size, const float2& position,
					  gui_pop_up_button* pop_up_button_,
					  const float2& button_position_abs_, const float2& button_size_abs_,
					  const vector<pair<const string, string>*>& items_)
	: gui_window(size, position), pop_up_button(pop_up_button_),
	button_position_abs(button_position_abs_), button_size_abs(button_size_abs_), items(items_) {
		margin = roundf(gui_theme::point_to_pixel(margin));
	}
	virtual ~gui_pop_up_window() {
		for(auto& cached_item : item_text_cache) {
			font::destroy_text_cache(cached_item.second.first);
		}
	}
	virtual void draw();
	
	virtual bool handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point);
	
protected:
	gui_pop_up_button* pop_up_button;
	const float2 button_position_abs;
	const float2 button_size_abs;
	const vector<pair<const string, string>*>& items;
	// item ptr -> <cachce, item height>
	unordered_map<pair<const string, string>*, pair<font::text_cache, float2>> item_text_cache;
	
	float margin = 3.5f; // in pt
	float2 overlay_size;
	float2 overlay_position;
	
};

void gui_pop_up_window::draw() {
	if(!state.visible) return;
	// TODO: only redraw when necessary
	
	start_draw();
	buffer.clear();
	oclraster_support::get_pipeline()->start_orthographic_rendering();
	
	// cache items
	for(const auto& item : items) {
		if(item_text_cache.find(item) == item_text_cache.end()) {
			const auto advance_map(fnt->compute_advance_map(item->second, 1));
			const float height = (advance_map.back().x < advance_map.back().y ?
								  (advance_map.back().y - advance_map.back().x) :
								  (advance_map.back().x + advance_map.back().y));
			item_text_cache.insert(make_pair(item,
											 make_pair(fnt->cache_text(item->second),
													   float2(height, 0.0f))));
		}
	}
	
	//
	const float item_margin = roundf(gui_theme::point_to_pixel(3.5f)); // in pt
	const float2 text_margin(roundf(gui_theme::point_to_pixel(5.0f)),
							 roundf(gui_theme::point_to_pixel(-1.5f))); // in pt
	
	const auto selected_item = pop_up_button->get_selected_item();
	float total_height = margin * 2.0f;
	float selected_item_height = 0.0f;
	for(const auto& item : items) {
		if(item == selected_item) {
			selected_item_height = total_height;
		}
		total_height += ceilf(item_text_cache.at(item).second.x + item_margin * 2.0f);
	}
	
	// compute overlay position and size
	overlay_size = float2(button_size_abs.x, total_height);
	
	float2 offset(button_position_abs - float2(margin, 0.0f));
	offset.y -= selected_item_height - margin;
	// clamp to (0, window size - overlay size) to prevent drawing outside of the window
	offset.clamp(float2(0.0f), float2(buffer.get_size()) - overlay_size);
	overlay_position = offset;
	
	// draw overlay rectangle
	theme->draw("pop_up_button", "overlay",
				overlay_position, overlay_size,
				false);
	
	// draw items
	const pnt mouse_pos(floor::get_event()->get_mouse_pos());
	float y_offset = margin + overlay_position.y;
	const float4 font_color(theme->get_color_scheme().get("TEXT_INVERSE"));
	const float4 font_color_active(theme->get_color_scheme().get("TEXT"));
	for(const auto& item : items) {
		//
		auto& cached_item(item_text_cache.at(item));
		const float text_height = cached_item.second.x; // text y-extent
		const float item_height = ceilf(text_height + item_margin * 2.0f);
		const float2 item_position(overlay_position.x, y_offset);
		const float2 item_size(button_size_abs.x, item_height);
		cached_item.second.y = item_height; // cache item height
		y_offset += item_height;
		
		if(mouse_pos.x > item_position.x && mouse_pos.x <= (item_position.x + item_size.x) &&
		   mouse_pos.y > item_position.y && mouse_pos.y <= (item_position.y + item_size.y)) {
			// active item
			theme->draw("pop_up_button", "item_active",
						item_position, item_size, false);
			fnt->draw_cached(cached_item.first.first.first, cached_item.first.first.second,
							 (item_position + text_margin).rounded(),
							 font_color_active);
		}
		else {
			fnt->draw_cached(cached_item.first.first.first, cached_item.first.first.second,
							 (item_position + text_margin).rounded(),
							 font_color);
		}
	}
	
	oclraster_support::get_pipeline()->stop_orthographic_rendering();
	stop_draw();
}

bool gui_pop_up_window::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj floor_unused, const ipnt& point) {
	// handle select
	if(type == EVENT_TYPE::MOUSE_LEFT_UP &&
	   point.x >= 0 &&
	   point.x > overlay_position.x &&
	   point.x <= (overlay_position.x + overlay_size.x)) {
		const float ypos(point.y - overlay_position.y - margin);
		float height = 0.0f;
		for(const auto& item : items) {
			const auto& cached_item(item_text_cache.at(item));
			if(ypos > height && ypos <= (height + cached_item.second.y)) {
				pop_up_button->set_selected_item(item->first, true);
				break;
			}
			height += cached_item.second.y;
		}
		// close will happen shortly after, but we don't it to be drawn any longer -> make invisible
		set_visible(false);
		pop_up_button->set_active(false);
		return true;
	}
	else if((type & EVENT_TYPE::__MOUSE_EVENT) == EVENT_TYPE::__MOUSE_EVENT &&
			type != EVENT_TYPE::MOUSE_MOVE &&
			type != EVENT_TYPE::MOUSE_WHEEL_DOWN &&
			type != EVENT_TYPE::MOUSE_WHEEL_UP) {
		// close overlay on all mouse events (except for move and mouse wheel events)
		set_visible(false);
		pop_up_button->set_active(false);
		return true;
	}
	return false;
}

////
gui_pop_up_button::gui_pop_up_button(const float2& size_, const float2& position_) :
gui_item_container(size_, position_, GUI_EVENT::POP_UP_BUTTON_SELECT) {
}

gui_pop_up_button::~gui_pop_up_button() {
}

void gui_pop_up_button::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	theme->draw("pop_up_button", state.active ? "active" : "normal",
				position_abs, size_abs, true, true,
				[this](const string& str floor_unused) -> string {
					if(selected_item == nullptr) return "";
					return selected_item->second;
				});
}


bool gui_pop_up_button::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj floor_unused, const ipnt& point floor_unused) {
	if(!state.visible || !state.enabled) return false;
	switch(type) {
		case EVENT_TYPE::MOUSE_LEFT_DOWN:
			set_active(true);
			return true;
		// mouse up / window close will be handled by gui_pop_up_window (or gui)
		default: break;
	}
	return false;
}

void gui_pop_up_button::set_active(const bool& active_state) {
	if(active_state == state.active) return;
	gui_object::set_active(active_state);
	
	if(active_state) {
		gui_object::handle(GUI_EVENT::POP_UP_BUTTON_ACTIVATION);
		open_selection_wnd();
	}
	else {
		gui_object::handle(GUI_EVENT::POP_UP_BUTTON_DEACTIVATION);
		close_selection_wnd();
	}
}

void gui_pop_up_button::open_selection_wnd() {
	task::spawn([this]() {
		floor::acquire_context();
		ui->lock();
		selection_wnd = ui->add<gui_pop_up_window>(float2(1.0f), float2(0.0f), this,
												   rel_to_abs_position(position_abs), size_abs,
												   display_items);
		ui->unlock();
		floor::release_context();
	});
}

void gui_pop_up_button::close_selection_wnd() {
	if(selection_wnd == nullptr) return;
	task::spawn([this]() {
		floor::acquire_context();
		ui->lock();
		ui->remove(selection_wnd);
		selection_wnd = nullptr;
		ui->unlock();
		floor::release_context();
	});
}
