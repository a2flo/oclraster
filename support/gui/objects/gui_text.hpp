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

#ifndef __OCLRASTER_SUPPORT_GUI_TEXT_H__
#define __OCLRASTER_SUPPORT_GUI_TEXT_H__

#include "gui/objects/gui_object.h"

class OCLRASTER_API gui_text : public gui_object {
public:
	gui_text(const float2& size, const float2& position);
	virtual ~gui_text();
	
	virtual void draw();
	
	void set_label(const string& label);
	const string& get_label() const;
	
	virtual void set_shade(const bool& state);
	virtual const bool& get_shade() const;
	
protected:
	string label = "";
	bool shade = false;

};

#endif
