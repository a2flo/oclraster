/*
 *  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
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

#ifndef __OCLRASTER_FRAMEBUFFER_H__
#define __OCLRASTER_FRAMEBUFFER_H__

#include "cl/opencl.h"
#include "pipeline/image.h"

class framebuffer {
public:
	//! NOTE: depth_type must either be NONE or { IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R }
	//! NOTE: stencil_type must either be NONE or { IMAGE_TYPE::UINT_*, IMAGE_CHANNEL::R }
	framebuffer(const unsigned int& width, const unsigned int& height,
				initializer_list<pair<IMAGE_TYPE, IMAGE_CHANNEL>> image_types,
				pair<IMAGE_TYPE, IMAGE_CHANNEL> depth_type = { IMAGE_TYPE::NONE, IMAGE_CHANNEL::NONE },
				pair<IMAGE_TYPE, IMAGE_CHANNEL> stencil_type = { IMAGE_TYPE::NONE, IMAGE_CHANNEL::NONE });
	~framebuffer();
	
	const uint2& get_size() const;
	
	const image* get_image(const size_t& index) const;
	image* get_image(const size_t& index);
	
	const image* get_depth_buffer() const;
	image* get_depth_buffer();
	
	const image* get_stencil_buffer() const;
	image* get_stencil_buffer();
	
protected:
	const uint2 size;
	vector<image*> images;
	image* depth_buffer = nullptr;
	image* stencil_buffer = nullptr;
	
};

#endif
