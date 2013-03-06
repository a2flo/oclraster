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

#ifndef __OCLRASTER_IMAGE_H__
#define __OCLRASTER_IMAGE_H__

#include "cl/opencl.h"
#include "pipeline/image_types.h"

class image {
public:
	//
	image(const unsigned int& width, const unsigned int& height,
		  const IMAGE_TYPE& type, const IMAGE_CHANNEL& channel_order,
		  const void* pixels = nullptr);
	image(image&& img);
	~image();
	
	// this uses SDL2_image to create an image from a .png file
	static image from_file(const string& filename,
						   const IMAGE_TYPE& type, const IMAGE_CHANNEL& channel_order);
	
	//
	enum class BACKING : unsigned int {
		BUFFER,	//!< backed by a simple opencl buffer
		IMAGE	//!< backed by an actual opencl image object
	};
	BACKING get_backing() const;
	
	IMAGE_TYPE get_data_type() const;
	IMAGE_CHANNEL get_channel_order() const;
	
	// note: opencl only supports read_only and write_only images
	// -> if you need read_write access inside your kernel,
	// buffer based backing will be used
	enum class KERNEL_ACCESS_TYPE : unsigned int {
		READ_ONLY	= (1u << 0u),
		WRITE_ONLY	= (1u << 1u),
		READ_WRITE	= (READ_ONLY | WRITE_ONLY)
	};
	
	// image header when a buffer is used
	struct __attribute__((packed, aligned(8))) header {
		IMAGE_TYPE type;
		IMAGE_CHANNEL channel_order;
		unsigned short int width;
		unsigned short int height;
	};
	static_assert(sizeof(header) == 8, "invalid image header size!");
	static constexpr size_t header_size() { return sizeof(header); }
	
	//
	const opencl::buffer_object* get_buffer() const;
	
protected:
	const BACKING backing;
	const IMAGE_TYPE data_type;
	const IMAGE_CHANNEL channel_order;
	opencl::buffer_object* buffer = nullptr;
	
};

#endif
