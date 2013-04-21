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

#include "gui_toggle_button.h"
#include "gui.h"

gui_toggle_button::gui_toggle_button(const float2& size_, const float2& position_) :
gui_object(size_, position_) {
	//
}

gui_toggle_button::~gui_toggle_button() {
}

void gui_toggle_button::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	theme->draw("toggle_button", toggled ? "toggled" : "normal",
				position_abs, size_abs, true, true,
				[this](const string& str oclr_unused) { return toggled ? toggled_label : untoggled_label; });
}

bool gui_toggle_button::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point oclr_unused) {
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
					set_toggled(toggled ^ true);
					handle(toggled ? GUI_EVENT::TOGGLE_BUTTON_ACTIVATION : GUI_EVENT::TOGGLE_BUTTON_DEACTIVATION);
				}
				return true;
			}
			break;
		}
		default: break;
	}
	return false;
}

void gui_toggle_button::set_label(const string& toggled_label_, const string& untoggled_label_) {
	toggled_label = toggled_label_;
	untoggled_label = untoggled_label_;
}

pair<const string&, const string&> gui_toggle_button::get_label() const {
	return { toggled_label, untoggled_label };
}

void gui_toggle_button::set_toggled(const bool& state) {
	if(state == toggled) return;
	toggled = state;
}

bool gui_toggle_button::is_toggled() const {
	return toggled;
}
