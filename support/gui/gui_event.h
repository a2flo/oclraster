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

#ifndef __OCLRASTER_SUPPORT_GUI_EVENT_H__
#define __OCLRASTER_SUPPORT_GUI_EVENT_H__

#include "core/event_objects.h"

// gui event types (addition to EVENT_TYPE)
enum class GUI_EVENT : unsigned int {
	BUTTON_PRESS = ((unsigned int)EVENT_TYPE::__GUI_EVENT) + 1u,
	
	TOGGLE_BUTTON_ACTIVATION,
	TOGGLE_BUTTON_DEACTIVATION,
	
	INPUT_BOX_ACTIVATION,
	INPUT_BOX_DEACTIVATION,
	INPUT_BOX_INPUT,
	INPUT_BOX_ENTER,
	
	POP_UP_BUTTON_ACTIVATION,
	POP_UP_BUTTON_DEACTIVATION,
	POP_UP_BUTTON_SELECT,
	
	SLIDER_ACTIVATION,
	SLIDER_DEACTIVATION,
	SLIDER_MOVE,
	
	LIST_BOX_SELECT,
	
	//CHECKBOX_TOGGLE,
	
	//RADIO_PRESS,
	
	//INPUT_SELECT,
	//INPUT_UNSELECT,
	
	//BAR_SCROLL,
	
	//FILE_OPEN,
	//FILE_SAVE,
	
	// TODO: tree list
	// TODO: color picker
	// TODO: progress bar
	// TODO: date picker
	
	WINDOW_OPEN,
	WINDOW_CLOSE,
	
	//TAB_SELECT,
};
namespace std {
	template <> struct hash<GUI_EVENT> : public hash<unsigned int> {
		size_t operator()(GUI_EVENT type) const throw() {
			return hash<unsigned int>::operator()((unsigned int)type);
		}
	};
}

class gui_object;
template<EVENT_TYPE event_type> struct gui_event_base : public event_object_base<event_type> {
	const gui_object& object;
	gui_event_base(const unsigned int& time_, const gui_object& object_) : event_object_base<event_type>(time_), object(object_) {}
};
typedef gui_event_base<EVENT_TYPE::__GUI_EVENT> gui_event;

#endif
