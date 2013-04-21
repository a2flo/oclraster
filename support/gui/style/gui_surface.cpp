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

#include "gui_surface.h"
#include "gui.h"
#include "rendering/gfx2d.h"
#include "rendering/shader.h"
#include "oclraster_support.h"

gui_surface::gui_surface(const float2& buffer_size_, const float2& offset_, const SURFACE_FLAGS flags_) :
flags(flags_), buffer_size(buffer_size_), buffer(0, 0), offset(offset_) {
	vbo_rectangle = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
									   opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
									   sizeof(float4) * 4);
	const array<unsigned int, 4> indices {{ 0, 1, 2, 3 }};
	rectangle_indices = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
										   opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
										   opencl::BUFFER_FLAG::INITIAL_COPY,
										   sizeof(unsigned int) * 4, &indices[0]);
	resize(buffer_size);
}

gui_surface::~gui_surface() {
	delete_buffer();
	if(vbo_rectangle != nullptr) ocl->delete_buffer(vbo_rectangle);
	if(rectangle_indices != nullptr) ocl->delete_buffer(rectangle_indices);
}

void gui_surface::delete_buffer() {
	framebuffer::destroy_images(buffer);
}

void gui_surface::resize(const float2& buffer_size_) {
	uint2 buffer_size_abs_ = ((flags & SURFACE_FLAGS::ABSOLUTE_SIZE) == SURFACE_FLAGS::ABSOLUTE_SIZE ?
							  buffer_size_.rounded() :
							  buffer_size_ * float2(oclraster::get_width(), oclraster::get_height()));
	if(buffer.get_attachment_count() != 0 &&
	   buffer_size_abs.x == buffer_size_abs_.x && buffer_size_abs.y == buffer_size_abs_.y) {
		// same size, nothing to do here
		return;
	}
	buffer_size = buffer_size_;
	buffer_size_abs = buffer_size_abs_;
	
	delete_buffer();
	
	const bool has_depth = ((flags & SURFACE_FLAGS::NO_DEPTH) != SURFACE_FLAGS::NO_DEPTH);
	buffer = framebuffer::create_with_images(buffer_size_abs.x, buffer_size_abs.y,
											 { { IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA } },
											 {
												 has_depth ? IMAGE_TYPE::FLOAT_32 : IMAGE_TYPE::NONE,
												 has_depth ? IMAGE_CHANNEL::R : IMAGE_CHANNEL::NONE
											 });
	
	// set blit vbo rectangle data
	set_offset(offset);
	
	//
	redraw();
}

void gui_surface::redraw() {
	do_redraw = true;
}

bool gui_surface::needs_redraw() const {
	return do_redraw;
}

const framebuffer* gui_surface::get_buffer() const {
	return &buffer;
}

const float2& gui_surface::get_buffer_size() const {
	return buffer_size;
}

void gui_surface::blit() {
	pipeline* p = oclraster_support::get_pipeline();
	p->bind_buffer("index_buffer", *rectangle_indices);
	p->bind_buffer("input_attributes", *vbo_rectangle);
	p->bind_image("texture", *buffer.get_image(0));
	p->draw(PRIMITIVE_TYPE::TRIANGLE_STRIP, 4, { 0, 2 });
}

const float4& gui_surface::get_extent() const {
	return extent;
}

void gui_surface::set_offset(const float2& offset_) {
	// TODO: make this context-less (rather set the offset as a shader uniform)
	
	// set blit vbo rectangle data
	offset = offset_;
	uint2 offset_abs = offset * float2(oclraster::get_width(), oclraster::get_height());
	extent.set(offset_abs.x, offset_abs.y, offset_abs.x + buffer.get_size().x, offset_abs.y + buffer.get_size().y);
	const array<float4, 4> points {
		{
			float4(extent.x, extent.w, 0.0f, 1.0f),
			float4(extent.x, extent.y, 0.0f, 1.0f),
			float4(extent.z, extent.w, 0.0f, 1.0f),
			float4(extent.z, extent.y, 0.0f, 1.0f)
		}
	};
	ocl->write_buffer(vbo_rectangle, &points[0]);
}

const float2& gui_surface::get_offset() const {
	return offset;
}

void gui_surface::start_draw() {
	oclraster_support::get_pipeline()->bind_framebuffer(&buffer);
}

void gui_surface::stop_draw() {
	oclraster_support::get_pipeline()->bind_framebuffer(nullptr);
}

void gui_surface::set_flags(const gui_surface::SURFACE_FLAGS& flags_) {
	flags = flags_;
}

const gui_surface::SURFACE_FLAGS& gui_surface::get_flags() const {
	return flags;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
gui_simple_callback::gui_simple_callback(ui_draw_callback& callback_, const DRAW_MODE_UI& mode_,
										 const float2& buffer_size_, const float2& offset_,
										 const SURFACE_FLAGS flags_) :
gui_surface(buffer_size_, offset_, flags_), mode(mode_), callback(&callback_) {
}

void gui_simple_callback::draw() {
	if(!do_redraw) return;
	
	start_draw();
	buffer.clear();
	oclraster_support::get_pipeline()->start_orthographic_rendering();
	
	(*callback)(mode, &buffer);
	
	oclraster_support::get_pipeline()->stop_orthographic_rendering();
	stop_draw();
	
	do_redraw = false;
}
