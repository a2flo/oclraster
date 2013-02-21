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

class image {
public:
	// this directly maps to opencl types (6.1.1)
	enum class TYPE : unsigned int {
		INT_8,		//!< char
		INT_16,		//!< short
		INT_32,		//!< int
		INT_64,		//!< long
		UINT_8,		//!< uchar
		UINT_16,	//!< ushort
		UINT_32,	//!< uint
		UINT_64,	//!< ulong
		FLOAT_16,	//!< half
		FLOAT_32,	//!< float
		FLOAT_64,	//!< double (note: must be supported by the device)
		__MAX_TYPE
	};
	enum class CHANNEL : unsigned int {
		R,
		RG,
		RGB,
		RGBA,
		__MAX_CHANNEL
	};
	
	image(const unsigned int& width, const unsigned int& height,
		  const TYPE& type, const CHANNEL& channel_order,
		  const void* pixels = nullptr);
	image(image&& img);
	~image();
	
	// this uses SDL2_image to create an image from a .png file
	static image from_file(const string& filename,
						   const TYPE& type, const CHANNEL& channel_order);
	
	//
	enum class BACKING : unsigned int {
		BUFFER,	//!< backed by a simple opencl buffer
		IMAGE	//!< backed by an actual opencl image object
	};
	BACKING get_backing() const;
	
	// note: opencl only supports read_only and write_only images
	// -> if you need read_write access inside your kernel,
	// buffer based backing will be used
	enum class KERNEL_ACCESS_TYPE : unsigned int {
		READ_ONLY	= (1u << 0u),
		WRITE_ONLY	= (1u << 1u),
		READ_WRITE	= (READ_ONLY | WRITE_ONLY)
	};
	
	// image header when a buffer is used
	struct __attribute__((packed, aligned(16))) header {
		TYPE type;
		CHANNEL channel_order;
		unsigned int width;
		unsigned int height;
	};
	static_assert(sizeof(header) == 16, "invalid image header size!");
	static constexpr size_t header_size() { return sizeof(header); }
	
	//
	const opencl::buffer_object* get_buffer() const;
	
protected:
	BACKING backing = BACKING::BUFFER;
	opencl::buffer_object* buffer = nullptr;
	
};

#endif
