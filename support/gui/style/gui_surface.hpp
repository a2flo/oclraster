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

#ifndef __OCLRASTER_SUPPORT_GUI_SURFACE_HPP__
#define __OCLRASTER_SUPPORT_GUI_SURFACE_HPP__

#include "oclraster_support/global.hpp"
#include "core/vector2.hpp"
#include <oclraster/pipeline/framebuffer.hpp>

class gui_surface {
public:
	enum class SURFACE_FLAGS : unsigned int {
		NONE				= 0,
		NO_ANTI_ALIASING	= (1 << 0),
		NO_DEPTH			= (1 << 1),
		ABSOLUTE_SIZE		= (1 << 2),
	};
	enum_class_bitwise_and(SURFACE_FLAGS)
	enum_class_bitwise_or(SURFACE_FLAGS)
	
	gui_surface(const float2& buffer_size, const float2& offset, const SURFACE_FLAGS flags = SURFACE_FLAGS::NONE);
	virtual ~gui_surface();
	
	virtual void draw() = 0;
	
	virtual void redraw();
	virtual bool needs_redraw() const;
	
	virtual void resize(const float2& buffer_size);
	virtual const float2& get_buffer_size() const;
	
	virtual void set_offset(const float2& offset);
	virtual const float2& get_offset() const;
	
	const float4& get_extent() const;
	
	const framebuffer* get_buffer() const;
	
	void blit();
	
	void set_flags(const SURFACE_FLAGS& flags);
	const SURFACE_FLAGS& get_flags() const;
	
protected:
	SURFACE_FLAGS flags;
	float2 buffer_size;
	uint2 buffer_size_abs;
	framebuffer buffer;
	
	float2 offset;
	float4 extent;
	opencl::buffer_object* vbo_rectangle = nullptr;
	opencl::buffer_object* rectangle_indices = nullptr;
	
	atomic<bool> do_redraw { false };
	
	void start_draw();
	void stop_draw();
	void delete_buffer();
	
};

// this is a simple gui_surface implementation that is used for user gui callbacks
// these are relatively simple and just need to draw into a buffer (with manual updating).
enum class DRAW_MODE_UI : unsigned int;
class gui_simple_callback : public gui_surface {
public:
	// note: typedef functor<void, const DRAW_MODE_UI, framebuffer*> ui_draw_callback;
	gui_simple_callback(functor<void, const DRAW_MODE_UI, framebuffer*>& callback,
						const DRAW_MODE_UI& mode,
						const float2& buffer_size, const float2& offset,
						const SURFACE_FLAGS flags = SURFACE_FLAGS::NONE);

	virtual void draw();
	
protected:
	const DRAW_MODE_UI mode;
	
	functor<void, const DRAW_MODE_UI, framebuffer*>* callback;
	
};

#endif
