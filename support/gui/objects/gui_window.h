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

#ifndef __OCLRASTER_SUPPORT_GUI_WINDOW_H__
#define __OCLRASTER_SUPPORT_GUI_WINDOW_H__

#include "gui/objects/gui_object.h"
#include "gui/style/gui_surface.h"

class OCLRASTER_API gui_window : public gui_object, public gui_surface {
public:
	gui_window(const float2& buffer_size, const float2& position);
	virtual ~gui_window();
	
	virtual void draw();
	
	// takes care of both gui_object and gui_surface functions,
	// which serve the same purpose in case of gui_window
	virtual void redraw();
	virtual bool needs_redraw() const;
	
	virtual void resize(const float2& buffer_size); // from gui_surface
	virtual void set_size(const float2& size); // from gui_object
	virtual void set_position(const float2& position); // from gui_object
	
	virtual ipnt abs_to_rel_position(const ipnt& point) const;
	virtual ipnt rel_to_abs_position(const ipnt& point) const;
	
	virtual bool handle_mouse_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj, const ipnt& point);
	virtual bool handle_key_event(const EVENT_TYPE& type, const shared_ptr<event_object>& obj);
	
	virtual void clear(const bool delete_children = true);

protected:

};

#endif
