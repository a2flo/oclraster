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

#ifndef __OCLRASTER_SUPPORT_GUI_INPUT_BOX_HPP__
#define __OCLRASTER_SUPPORT_GUI_INPUT_BOX_HPP__

#include "gui/objects/gui_object.hpp"

class FLOOR_API gui_input_box : public gui_object {
public:
	gui_input_box(const float2& size, const float2& position);
	virtual ~gui_input_box();
	
	virtual void draw();
	
	virtual bool handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point);
	virtual bool handle_key_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj);
	
	void set_input(const string& input);
	const string& get_input() const;
	
	virtual void compute_abs_values();
	virtual void set_active(const bool& state);
	
protected:
	string input = "";
	vector<unsigned int> unicode_input;
	vector<float2> advance_map { float2(0.0f) };
	void update_input();
	void update_text_display();
	
	float2 input_text_margin = float2(3.0f, 1.5f); // in pt
	float2 input_size_abs;
	float2 input_text_position;
	float2 input_text_interval;
	float cursor_position = 0.0f;
	
	ssize_t input_cursor = -1; // -1 -> last
	
	atomic<bool> blink_state { false };
	atomic<bool> blink_task { false };
	atomic<bool> blink_task_end { true };

};

#endif
