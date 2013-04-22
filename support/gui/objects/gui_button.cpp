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

#include "gui_button.h"
#include "gui.h"

gui_button::gui_button(const float2& size_, const float2& position_) :
gui_text(size_, position_) {
	//
}

gui_button::~gui_button() {
}

void gui_button::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	theme->draw("button", state.active ? "active" : "normal",
				position_abs, size_abs, true, true,
				[this](const string& str oclr_unused) { return label; });
}

bool gui_button::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point oclr_unused) {
	if(!state.visible || !state.enabled) return false;
	switch(type) {
		case EVENT_TYPE::MOUSE_LEFT_DOWN:
			ui->set_active_object(this);
			return true;
		case EVENT_TYPE::MOUSE_LEFT_CLICK:
		case EVENT_TYPE::MOUSE_LEFT_DOUBLE_CLICK: {
			if(state.active) {
				ui->set_active_object(nullptr);
				
				// down position has already been checked (we wouldn't be in here otherwise)
				// -> check if the up position is also within the button, if so, we have a button click
				const auto& click_event = (const mouse_left_click_event&)*obj;
				const ipnt up_position(abs_to_rel_position(click_event.up->position));
				if(gfx2d::is_pnt_in_rectangle(rectangle_abs, up_position)) {
					// handle click
					handle(GUI_EVENT::BUTTON_PRESS);
				}
				return true;
			}
			break;
		}
		default: break;
	}
	return false;
}
