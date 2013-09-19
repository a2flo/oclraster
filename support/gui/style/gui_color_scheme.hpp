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

#ifndef __OCLRASTER_SUPPORT_GUI_COLOR_SCHEME_H__
#define __OCLRASTER_SUPPORT_GUI_COLOR_SCHEME_H__

#include "oclraster_support/global.h"
#include "core/xml.h"

class xml;
class gui_color_scheme {
public:
	gui_color_scheme();
	~gui_color_scheme();
	
	bool load(const string& filename);
	
	const float4 get(const string& name) const;
	
protected:
	xml* x;
	
	unordered_map<string, float4> colors;
	
	void process_node(const xml::xml_node* node, const xml::xml_node* parent);
	
};

#endif
