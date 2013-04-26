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
	enum class BACKING : unsigned int {
		BUFFER,	//!< backed by a simple opencl buffer
		IMAGE	//!< backed by an actual opencl image object
	};
	
	// constructor for both buffer backed and image backed images
	image(const unsigned int& width, const unsigned int& height,
		  const BACKING& backing,
		  const IMAGE_TYPE& type,
		  const IMAGE_CHANNEL& channel_order,
		  const void* pixels = nullptr);
	// alternate constructor for image backed images (for directly specifying the native format)
	image(const unsigned int& width, const unsigned int& height,
		  const IMAGE_TYPE& type,
		  const IMAGE_CHANNEL& channel_order,
		  const cl::ImageFormat& native_format,
		  const void* pixels = nullptr);
	image(image&& img);
	~image();
	
	// this uses SDL2_image to create an image from a .png file
	static image from_file(const string& filename, const BACKING& backing,
						   const IMAGE_TYPE& type, const IMAGE_CHANNEL& channel_order);
	
	//
	BACKING get_backing() const;
	image_type get_image_type() const;
	IMAGE_TYPE get_data_type() const;
	IMAGE_CHANNEL get_channel_order() const;
	const uint2& get_size() const;
	const cl::ImageFormat& get_native_format() const;
	
	// note: default parameters => complete image
	void write(const void* src,
			   const uint2 offset = { 0u, 0u },
			   const uint2 size = { ~0u, ~0u });
	void read(void* dst,
			  const uint2 offset = { 0u, 0u },
			  const uint2 size = { ~0u, ~0u });
	void copy(const image& src_img,
			  const uint2 src_offset = { 0u, 0u },
			  const uint2 dst_offset = { 0u, 0u },
			  const uint2 size = { ~0u, ~0u });
	void* __attribute__((aligned(128))) map(const opencl::MAP_BUFFER_FLAG access_type =
											opencl::MAP_BUFFER_FLAG::READ_WRITE |
											opencl::MAP_BUFFER_FLAG::BLOCK);
	const void* __attribute__((aligned(128))) map(const opencl::MAP_BUFFER_FLAG access_type =
												  opencl::MAP_BUFFER_FLAG::READ |
												  opencl::MAP_BUFFER_FLAG::BLOCK) const; // read-only!
	// note: if buffer based backing is used, offset.x must be 0 and size.x must match the image width!
	void* __attribute__((aligned(128))) map_region(const uint2 offset = { 0u, 0u },
												   const uint2 size = { ~0u, ~0u },
												   const opencl::MAP_BUFFER_FLAG access_type =
												   opencl::MAP_BUFFER_FLAG::READ_WRITE |
												   opencl::MAP_BUFFER_FLAG::BLOCK);
	void unmap(const void* mapped_ptr) const;
	
	// use this function to convert the image between BUFFER and IMAGE based backing
	// note that this will of course create a new buffer/image and copy the data
	bool modify_backing(const BACKING& new_backing);
	
	// note: opencl only supports read_only and write_only images
	// -> if you need read_write access inside your kernel,
	// buffer based backing must be used
	enum class KERNEL_ACCESS_TYPE : unsigned int {
		READ_ONLY	= (1u << 0u),
		WRITE_ONLY	= (1u << 1u),
		READ_WRITE	= (READ_ONLY | WRITE_ONLY)
	};
	
	// image header when a buffer is used
#define OCLRASTER_IMAGE_HEADER_SIZE 4096
	struct __attribute__((packed, aligned(OCLRASTER_IMAGE_HEADER_SIZE))) header {
		IMAGE_TYPE type;
		IMAGE_CHANNEL channel_order;
		unsigned short int width;
		unsigned short int height;
		unsigned char _unused[OCLRASTER_IMAGE_HEADER_SIZE - 8];
	};
	static constexpr size_t header_size() {
		// device specific address alignment (min 128 bytes, but can be up to 4096 -> use 4096)
		static_assert(sizeof(header) == (OCLRASTER_IMAGE_HEADER_SIZE), "invalid image header size!");
		return (OCLRASTER_IMAGE_HEADER_SIZE);
	}
	
	//
	const opencl::buffer_object* get_buffer() const;
	opencl::buffer_object* get_buffer();
	
	// only available if buffer based backing is used
	const opencl::buffer_object* get_data_buffer() const;
	opencl::buffer_object* get_data_buffer();
	
	//
	void invalidate();
	bool is_valid() const;
	
protected:
	BACKING backing;
	image_type img_type;
	const IMAGE_TYPE data_type;
	const IMAGE_CHANNEL channel_order;
	const uint2 size;
	opencl::buffer_object* buffer = nullptr;
	bool valid = false;
	
	// only used with image based backing
	cl::ImageFormat native_format;
	
	// only used with buffer based backing
	// sub-buffer of "buffer", only containing the image data
	opencl::buffer_object* data_buffer = nullptr;
	
	//
	void create_buffer(const void* pixels);
	
};

#endif
