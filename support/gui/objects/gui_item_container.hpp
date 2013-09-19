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

#ifndef __OCLRASTER_SUPPORT_GUI_ITEM_CONTAINER_HPP__
#define __OCLRASTER_SUPPORT_GUI_ITEM_CONTAINER_HPP__

#include "gui/objects/gui_object.hpp"

// use this as the base class for all objects that need to store items (don't instantiate directly!)
class FLOOR_API gui_item_container : public gui_object {
public:
	gui_item_container(const float2& size, const float2& position, const GUI_EVENT select_event);
	virtual ~gui_item_container();
	
	virtual void clear();
	virtual void add_item(const string& identifier, const string& label);
	virtual void remove_item(const string& identifier);
	
	virtual const pair<const string, string>* get_selected_item() const;
	virtual void set_selected_item(const string& identifier, const bool event_on_equal = false);
	virtual void set_selected_item(const size_t& index, const bool event_on_equal = false);
	
protected:
	// <identifier, label>
	unordered_map<string, string> items;
	vector<typename decltype(items)::value_type*> display_items;
	typename decltype(items)::value_type* selected_item = nullptr;
	
	GUI_EVENT select_event;

};

#endif
