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

#ifndef __OCLRASTER_SAMPLE_VOLUME_H__
#define __OCLRASTER_SAMPLE_VOLUME_H__

#include <oclraster/oclraster.h>
#include <oclraster/pipeline/image.h>

#define LAYER_SIZE 3.0f

class volume {
public:
	static volume* from_file(const string& filename);
	
	volume(const unsigned char* volume_data, const uint3& volume_size);
	~volume();
	
	struct __attribute__((packed, aligned(16))) vertex_data {
		float4 vertex;
		float2 tex_coord;
	};
	
protected:
	array<image*, 3> stacks { { nullptr, nullptr, nullptr } }; // one for each axis
	uint3 size;
	
	array<opencl::buffer_object*, 3> vertex_buffer { { nullptr, nullptr, nullptr } };
	array<opencl::buffer_object*, 6> index_buffer { { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr } };
	size_t index_count[3];
	
	void construct_texture(const unsigned char* volume_data);
	void construct_volume_texture(unsigned char* volume_tex, const unsigned char* volume_data, const size_t& axis, const size_t data_offset = 0);
	void construct_buffers();
	
	const unsigned char* resize_volume(const unsigned char* volume_data);
	
public:
	image* get_texture(const size_t& axis) const {
		return stacks[axis];
	}
	
	const opencl::buffer_object& get_vertex_buffer(const size_t& axis) const {
		return *vertex_buffer[axis];
	}
	const opencl::buffer_object& get_index_buffer(const size_t& axis, const size_t& side) const {
		return *index_buffer[axis*2 + side];
	}
	size_t get_vertex_count(const size_t& axis) const {
		return size[axis] * 4;
	}
	size_t get_index_count(const size_t& axis) const {
		return index_count[axis];
	}
	
	const uint3& get_size() const { return size; }

};

#endif
