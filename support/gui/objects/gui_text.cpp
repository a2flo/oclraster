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

#include "gui_text.h"
#include "core/core.h"

gui_text::gui_text(const float2& size_, const float2& position_) :
gui_object(size_, position_) {
	//
}

gui_text::~gui_text() {
}

void gui_text::draw() {
	if(!gui_object::handle_draw()) return;
	
	// TODO: handle disabled state
	// TODO: handle centering
	theme->draw("text", shade ? "normal_shade" : "normal",
				position_abs, size_abs, true, true,
				[this](const string& str oclr_unused) { return label; });
}

void gui_text::set_label(const string& label_) {
	label = label_;
	redraw();
}

const string& gui_text::get_label() const {
	return label;
}

void gui_text::set_shade(const bool& state) {
	shade = state;
	redraw();
}

const bool& gui_text::get_shade() const {
	return shade;
}
