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

#ifndef __OCLRASTER_SUPPORT_GUI_LIST_BOX_HPP__
#define __OCLRASTER_SUPPORT_GUI_LIST_BOX_HPP__

#include "gui/objects/gui_item_container.hpp"

class FLOOR_API gui_list_box : public gui_item_container {
public:
	gui_list_box(const float2& size, const float2& position);
	virtual ~gui_list_box();
	
	virtual void draw();
	
	//
	virtual void clear();
	virtual void add_item(const string& identifier, const string& label);
	virtual void remove_item(const string& identifier);
	
	//
	virtual bool handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point);
	
protected:
	float item_height = 0.0f;
	float box_height = 0.0f;
	float scroll_position = 0.0f;
	
	void recompute_height();

};

#endif
