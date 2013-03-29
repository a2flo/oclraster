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

framebuffer framebuffer::create_with_images(const unsigned int& width, const unsigned int& height,
											initializer_list<pair<IMAGE_TYPE, IMAGE_CHANNEL>> image_types,
											pair<IMAGE_TYPE, IMAGE_CHANNEL> depth_type,
											pair<IMAGE_TYPE, IMAGE_CHANNEL> stencil_type) {
	//
	framebuffer ret_fb(width, height);
	
	// check validity
	for(const auto& img_type : image_types) {
		if(img_type.first == IMAGE_TYPE::NONE ||
		   img_type.second == IMAGE_CHANNEL::NONE) {
			oclr_error("framebuffer images without a type are not allowed!");
			return ret_fb;
		}
	}
	if(!((depth_type.first == IMAGE_TYPE::NONE && depth_type.second == IMAGE_CHANNEL::NONE) ||
		 (depth_type.first == IMAGE_TYPE::FLOAT_32 && depth_type.second == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer depth type must either be NONE or FLOAT_32/R");
		return ret_fb;
	}
	if(!((stencil_type.first == IMAGE_TYPE::NONE && stencil_type.second == IMAGE_CHANNEL::NONE) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_8 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_16 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_32 && stencil_type.second == IMAGE_CHANNEL::R) ||
		 (stencil_type.first == IMAGE_TYPE::UINT_64 && stencil_type.second == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer stencil type must either be NONE or UINT_*/R");
		return ret_fb;
	}
	
	// create all framebuffer images
	size_t img_idx = 0;
	for(const auto& img_type : image_types) {
		ret_fb.attach(img_idx, *new image(width, height, image::BACKING::BUFFER, img_type.first, img_type.second));
		img_idx++;
	}
	if(depth_type.first != IMAGE_TYPE::NONE) {
		ret_fb.attach_depth_buffer(*new image(width, height, image::BACKING::BUFFER, depth_type.first, depth_type.second));
	}
	if(stencil_type.first != IMAGE_TYPE::NONE) {
		ret_fb.attach_stencil_buffer(*new image(width, height, image::BACKING::BUFFER, stencil_type.first, stencil_type.second));
	}
	
	return ret_fb;
}

void framebuffer::destroy_images(framebuffer& fb) {
	for(const auto& img : fb.images) {
		if(img != nullptr) delete img;
	}
	if(fb.depth_buffer != nullptr) delete fb.depth_buffer;
	if(fb.stencil_buffer != nullptr) delete fb.stencil_buffer;
}

framebuffer::framebuffer(const unsigned int& width, const unsigned int& height) : size(width, height) {
}

framebuffer::~framebuffer() {
}

framebuffer::framebuffer(framebuffer&& fb) : size(fb.size), images(fb.images), depth_buffer(fb.depth_buffer), stencil_buffer(fb.stencil_buffer) {
	fb.images.clear();
	fb.depth_buffer = nullptr;
	fb.stencil_buffer = nullptr;
}

framebuffer& framebuffer::operator=(framebuffer&& fb) {
	this->size = fb.size;
	this->images = std::move(fb.images);
	this->depth_buffer = fb.depth_buffer;
	this->stencil_buffer = fb.stencil_buffer;
	fb.depth_buffer = nullptr;
	fb.stencil_buffer = nullptr;
	return *this;
}

void framebuffer::set_size(const uint2& size_) {
	size = size_;
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

void framebuffer::attach(const size_t& index, image& img) {
	if(index >= images.size()) {
		images.resize(index+1, nullptr);
	}
	images[index] = &img;
}

void framebuffer::detach(const size_t& index) {
	const size_t attachment_count = images.size();
#if defined(OCLRASTER_DEBUG)
	if(index >= attachment_count) {
		oclr_error("invalid index: %u - current framebuffer attachment count is only %u!", index, attachment_count);
		return;
	}
#endif
	images[index] = nullptr;
	
	// cleanup
	if(index+1 == attachment_count) {
		size_t new_count = 0;
		for(ssize_t idx = index-1; idx >= 0; idx--) {
			if(images[idx] != nullptr) {
				new_count = idx+1;
			}
		}
		images.resize(new_count);
		images.shrink_to_fit();
	}
}

size_t framebuffer::get_attachment_count() const {
	return images.size();
}

void framebuffer::attach_depth_buffer(image& img) {
	if(!((img.get_data_type() == IMAGE_TYPE::NONE && img.get_channel_order() == IMAGE_CHANNEL::NONE) ||
		 (img.get_data_type() == IMAGE_TYPE::FLOAT_32 && img.get_channel_order() == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer depth type must either be NONE or FLOAT_32/R");
		return;
	}
	depth_buffer = &img;
}
void framebuffer::detach_depth_buffer() {
	depth_buffer = nullptr;
}

void framebuffer::attach_stencil_buffer(image& img) {
	if(!((img.get_data_type() == IMAGE_TYPE::NONE && img.get_channel_order() == IMAGE_CHANNEL::NONE) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_8 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_16 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_32 && img.get_channel_order() == IMAGE_CHANNEL::R) ||
		 (img.get_data_type() == IMAGE_TYPE::UINT_64 && img.get_channel_order() == IMAGE_CHANNEL::R))) {
		oclr_error("framebuffer stencil type must either be NONE or UINT_*/R");
		return;
	}
	stencil_buffer = &img;
}
void framebuffer::detach_stencil_buffer() {
	stencil_buffer = nullptr;
}
