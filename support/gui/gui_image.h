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

#ifndef __OCLRASTER_SUPPORT_GUI_IMAGE_H__
#define __OCLRASTER_SUPPORT_GUI_IMAGE_H__

#include "oclraster_support/global.h"
#include <oclraster/pipeline/image.h>

class OCLRASTER_API gui_image {
public:
	gui_image();
	~gui_image();

	void draw();
	void draw(unsigned int scale_x, unsigned int scale_y, bool flip_y = false);
	void open_image(const char* filename);

	void set_position(unsigned int x, unsigned int y);
	void set_position(pnt* position);
	pnt& get_position();

	const image* get_texture() const;
	void set_texture(const image& new_tex);

	void set_scaling(bool state);
	bool get_scaling();
	void set_gui_img(bool state);

	unsigned int get_width();
	unsigned int get_height();

	void set_color(unsigned int color);

protected:
	pnt position;

	const image* tex = nullptr;

	bool scale = true;
	bool gui_img = false;

	unsigned int color = 0xFFFFFF;

};

#endif
