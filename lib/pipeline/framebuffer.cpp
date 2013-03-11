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

#include "framebuffer.h"

framebuffer::framebuffer(const unsigned int& width, const unsigned int& height,
						 initializer_list<pair<IMAGE_TYPE, IMAGE_CHANNEL>> image_types,
						 pair<IMAGE_TYPE, IMAGE_CHANNEL> depth_type,
						 pair<IMAGE_TYPE, IMAGE_CHANNEL> stencil_type) : size(width, height) {
	// check validity
	for(const auto& img_type : image_types) {
		if(img_type.first == IMAGE_TYPE::NONE ||
		   img_type.second == IMAGE_CHANNEL::NONE) {
			oclr_error("framebuffer images without a type are not allowed!");
			return;
		}
	}
	if(!((depth_type.first == IMAGE_TYPE::NONE && depth_type.second == IMAGE_CHANNEL::NONE) ||
		 (depth_type.first == IMAGE_TYPE::FLOAT_32 && depth_type.second == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer depth type must either be NONE or FLOAT_32/R");
		return;
	}
	if(!((stencil_type.first == IMAGE_TYPE::NONE && stencil_type.second == IMAGE_CHANNEL::NONE) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_8 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_16 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_32 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_64 && stencil_type.second == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer stencil type must either be NONE or UINT_*/R");
		return;
	}
	
	// create all framebuffer images
	for(const auto& img_type : image_types) {
		images.push_back(new image(width, height, img_type.first, img_type.second));
	}
	if(depth_type.first != IMAGE_TYPE::NONE) {
		depth_buffer = new image(width, height, depth_type.first, depth_type.second);
	}
	if(stencil_type.first != IMAGE_TYPE::NONE) {
		stencil_buffer = new image(width, height, stencil_type.first, stencil_type.second);
	}
}

framebuffer::~framebuffer() {
	for(const auto& img : images) {
		delete img;
	}
	if(depth_buffer != nullptr) delete depth_buffer;
	if(stencil_buffer != nullptr) delete stencil_buffer;
}

const uint2& framebuffer::get_size() const {
	return size;
}

const image* framebuffer::get_image(const size_t& index) const {
	return (index >= images.size() ? nullptr : images[index]);
}

image* framebuffer::get_image(const size_t& index) {
	return (index >= images.size() ? nullptr : images[index]);
}

const image* framebuffer::get_depth_buffer() const {
	return depth_buffer;
}

image* framebuffer::get_depth_buffer() {
	return depth_buffer;
}

const image* framebuffer::get_stencil_buffer() const {
	return stencil_buffer;
}

image* framebuffer::get_stencil_buffer() {
	return stencil_buffer;
}