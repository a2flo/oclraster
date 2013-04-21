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

#include "gui_list_box.h"
#include "gui.h"
#include "font.h"

gui_list_box::gui_list_box(const float2& size_, const float2& position_) :
gui_item_container(size_, position_, GUI_EVENT::LIST_BOX_SELECT) {
	//
	const float item_margin = gui_theme::point_to_pixel(2.0f); // in pt
	item_height = fnt->get_display_size() + item_margin;
}

gui_list_box::~gui_list_box() {
}

void gui_list_box::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	theme->draw("list_box", "normal", position_abs, size_abs);
	
	// manual scissor test:
	glScissor(floorf(position_abs.x), floorf(position_abs.y),
			  ceilf(size_abs.x), ceilf(size_abs.y));
	
	size_t item_counter = 0;
	for(const auto& item : display_items) {
		// check if item is in box
		const float y_offset = float(item_counter) * item_height;
		const float y_next_offset = y_offset + item_height;
		item_counter++;
		if(y_next_offset < scroll_position && y_offset >= size_abs.y) {
			continue;
		}
		
		const float2 cur_position(position_abs + float2(0.0f, y_offset - scroll_position));
		theme->draw("list_box", item == selected_item ? "item_active" : "item",
					cur_position, float2(size_abs.x, item_height),
					false, false,
					[&item](const string&) -> string { return item->second; });
	}
	
	glScissor(0, 0, oclraster::get_width(), oclraster::get_height());
}

bool gui_list_box::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj oclr_unused, const ipnt& point) {
	if(!state.visible || !state.enabled) return false;
	switch(type) {
		case EVENT_TYPE::MOUSE_LEFT_DOWN: {
			ui->set_active_object(this);
			const ipnt pos_in_box = point - position_abs;
			const size_t select_item = floorf((float(pos_in_box.y) + scroll_position) / item_height);
			// this check is also done in set_selected_item, but since the item number can legitimately
			// be out of range, do this check here to avoid the log error
			if(select_item >= display_items.size()) {
				return false;
			}
			set_selected_item(select_item);
			return true;
		}
		case EVENT_TYPE::MOUSE_WHEEL_UP:
		case EVENT_TYPE::MOUSE_WHEEL_DOWN:
			if(box_height < size_abs.y) return false;
			scroll_position += item_height * 1.5f * (type == EVENT_TYPE::MOUSE_WHEEL_DOWN ? 1.0f : -1.0f);
			scroll_position = core::clamp(scroll_position, 0.0f, box_height-float(size_abs.y));
			return true;
		default: break;
	}
	return false;
}

void gui_list_box::clear() {
	gui_item_container::clear();
	recompute_height();
}

void gui_list_box::add_item(const string& identifier, const string& label) {
	gui_item_container::add_item(identifier, label);
	recompute_height();
}

void gui_list_box::remove_item(const string& identifier) {
	gui_item_container::remove_item(identifier);
	recompute_height();
}

void gui_list_box::recompute_height() {
	box_height = ceilf(item_height * float(display_items.size()));
	if(box_height < size_abs.y) {
		// reset scroll position if there are fewer items than the list can display
		scroll_position = 0.0f;
	}
}
