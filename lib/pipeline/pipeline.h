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

#ifndef __OCLRASTER_PIPELINE_H__
#define __OCLRASTER_PIPELINE_H__

#include "cl/opencl.h"
#include "pipeline/transform_stage.h"
#include "core/rtt.h"

class pipeline {
public:
	pipeline();
	~pipeline();
	
	struct draw_flags {
		// TODO: store actual flags/data
		union {
			struct {
				unsigned int blending : 1;
				unsigned int depth_test : 1;
				unsigned int scissor_test : 1;
				unsigned int backface_culling : 1;
				
				//
				unsigned int _unused : 28;
			};
			unsigned int flags;
		};
		
		// set this to the required size (transformed primitive + attribute data)
		unsigned int transformed_primitive_size = 3 * sizeof(float3);
	};
	
	// "draw calls" (for now, these always draw triangles)
	void draw(const transform_stage::vertex_buffer& vb,
			  const transform_stage::index_buffer& ib,
			  //const transform_stage::constant_buffer& cb,
			  const draw_flags& flags,
			  // default element range: draw all
			  const pair<unsigned int, unsigned int> element_range = { ~0u, ~0u });
	
	// for debugging and initial development purposes:
	void reserve_memory(const unsigned int triangle_count);
	
protected:
	opencl_base* ocl = nullptr;
	rtt::fbo* framebuffer = nullptr;
	opencl::buffer_object* framebuffer_cl = nullptr;
	
	//
	const uint2 tile_size = uint2(16, 16);
	opencl_base::buffer_object* triangle_queues_buffer = nullptr;
	opencl_base::buffer_object* queue_sizes_buffer = nullptr;
	unsigned int* triangle_queues_buffer_zero = nullptr;
	unsigned int* queue_sizes_buffer_zero = nullptr;
	unsigned int reserved_triangle_count = 0;
	
};

#endif
