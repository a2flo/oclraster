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
	// in general, the framebuffer does not manage the images memory and is only a container for image pointers
	// however, for making things easier in some situations, these are two helper functions that will also handle the images memory
	//! NOTE: depth_type must either be NONE or { IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R }
	//! NOTE: stencil_type must either be NONE or { IMAGE_TYPE::UINT_*, IMAGE_CHANNEL::R }
	static framebuffer create_with_images(const unsigned int& width, const unsigned int& height,
										  initializer_list<pair<IMAGE_TYPE, IMAGE_CHANNEL>> image_types,
										  pair<IMAGE_TYPE, IMAGE_CHANNEL> depth_type = { IMAGE_TYPE::NONE, IMAGE_CHANNEL::NONE },
										  pair<IMAGE_TYPE, IMAGE_CHANNEL> stencil_type = { IMAGE_TYPE::NONE, IMAGE_CHANNEL::NONE });
	static void destroy_images(framebuffer& fb);
	
	//
	framebuffer(const unsigned int& width, const unsigned int& height);
	~framebuffer();
	framebuffer(framebuffer&& fb);
	framebuffer& operator=(framebuffer&& fb);
	
	void set_size(const uint2& size);
	const uint2& get_size() const;
	
	const image* get_image(const size_t& index) const;
	image* get_image(const size_t& index);
	
	const image* get_depth_buffer() const;
	image* get_depth_buffer();
	
	const image* get_stencil_buffer() const;
	image* get_stencil_buffer();
	
	//
	void clear(const vector<size_t> image_indices = vector<size_t> { ~0u },
			   const bool clear_depth = true,
			   const bool clear_stencil = true) const;
	
	// clear colors/values are clamped to the used image format types
	// e.g.: UINT_8 is used, clear color is set to ~0 => will use 255u
	void set_clear_color(const ulong4 value = ulong4 { 0, 0, 0, 0 });
	const ulong4& get_clear_color() const;
	
	void set_clear_depth(const float value = std::numeric_limits<float>::max());
	const float& get_clear_depth() const;
	
	void set_clear_stencil(const unsigned long long int value = 0ull);
	const unsigned long long int& get_clear_stencil() const;
	
	//
	void attach(const size_t& index, image& img);
	void detach(const size_t& index);
	
	void attach_depth_buffer(image& img);
	void detach_depth_buffer();
	
	void attach_stencil_buffer(image& img);
	void detach_stencil_buffer();
	
	// does not include depth and stencil buffers
	size_t get_attachment_count() const;
	
protected:
	uint2 size;
	vector<image*> images;
	image* depth_buffer = nullptr;
	image* stencil_buffer = nullptr;
	
	//
	ulong4 clear_color { 0, 0, 0, 0 };
	float clear_depth { std::numeric_limits<float>::max() };
	unsigned long long int clear_stencil { 0 };
	
};

// only used internally!
extern void delete_clear_kernels();

#endif
