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

#include "gui_slider.hpp"
#include "gui.hpp"

gui_slider::gui_slider(const float2& size_, const float2& position_) :
gui_object(size_, position_) {
	//
	knob_radius = gui_theme::point_to_pixel(knob_radius);
	knob_offset = gui_theme::point_to_pixel(knob_offset);
}

gui_slider::~gui_slider() {
}

void gui_slider::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	theme->draw("slider", "normal",
				position_abs, size_abs);
	theme->draw("slider", state.active ? "active_knob" : "normal_knob",
				position_abs + float2((knob_offset - knob_radius) + knob_position * slider_width, 0.0f),
				float2(knob_radius * 2.0f, size_abs.y), false);
}

void gui_slider::compute_abs_values() {
	gui_object::compute_abs_values();
	slider_width = float(rectangle_abs.x2 - rectangle_abs.x1) - (knob_offset * 2.0f);
}

bool gui_slider::handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj floor_unused, const ipnt& point) {
	if(!state.visible || !state.enabled) return false;
	switch(type) {
		case EVENT_TYPE::MOUSE_LEFT_DOWN:
			ui->set_active_object(this);
			handle(GUI_EVENT::SLIDER_ACTIVATION);
			move_knob(point);
			return true;
		case EVENT_TYPE::MOUSE_LEFT_UP:
			ui->set_active_object(nullptr);
			handle(GUI_EVENT::SLIDER_DEACTIVATION);
			return true;
		case EVENT_TYPE::MOUSE_MOVE: {
			if(!state.active) return false;
			return move_knob(point);
		}
		default: break;
	}
	return false;
}

bool gui_slider::move_knob(const ipnt& point) {
	const int iknob_offset = ceilf(knob_offset);
	const int2 slider_x_pos((int)rectangle_abs.x1 + iknob_offset,
							(int)rectangle_abs.x2 - iknob_offset);
	if(point.x <= slider_x_pos.x && knob_position > 0.0f) {
		knob_position = 0.0f;
	}
	else if(point.x >= slider_x_pos.y && knob_position < 1.0f) {
		knob_position = 1.0f;
	}
	else if(point.x >= slider_x_pos.x && point.x <= slider_x_pos.y) {
		knob_position = float(point.x - slider_x_pos.x) / slider_width;
	}
	else {
		// no change!
		return false;
	}
	handle(GUI_EVENT::SLIDER_MOVE);
	return true;
}

bool gui_slider::should_handle_mouse_event(const EVENT_TYPE& type, const ipnt& point) const {
	if(state.active && (type == EVENT_TYPE::MOUSE_LEFT_UP || type == EVENT_TYPE::MOUSE_MOVE)) return true;
	return gui_object::should_handle_mouse_event(type, point);
}

void gui_slider::set_knob_position(const float& pos) {
	knob_position = core::clamp(pos, 0.0f, 1.0f);
	handle(GUI_EVENT::SLIDER_MOVE);
}

float gui_slider::get_knob_position() const {
	return knob_position;
}
