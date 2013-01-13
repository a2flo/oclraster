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

#include "rasterization_stage.h"
#include "pipeline.h"
#include "oclraster.h"

struct __attribute__((packed)) constant_data_rp {
	const unsigned int triangle_count;
};

rasterization_stage::rasterization_stage() {
	const_buffer_rp = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
										 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
										 sizeof(constant_data_rp), nullptr);
}

rasterization_stage::~rasterization_stage() {
	ocl->delete_buffer(const_buffer_rp);
}

void rasterization_stage::rasterize(draw_state& state,
									const unsigned int& triangle_count) {
	////
	// render / rasterization
	const constant_data_rp const_data_rp {
		triangle_count
	};
	ocl->write_buffer(const_buffer_rp, &const_data_rp); // TODO: make this non-blocking
	
	//
	static float light_pos = 0.0f, light_dist = 10.0f;
	struct __attribute__((packed, aligned(16))) oclraster_uniforms {
		float4 camera_position;
		float4 light_position; // .w = light radius ^ 2
		float4 light_color;
	} rasterize_uniforms {
		float4(oclraster::get_camera_setup().position, 1.0f),
		float4(sinf(light_pos)*light_dist, 0.0f, cosf(light_pos)*light_dist, 16.0f*16.0f),
		float4(0.0f, 0.3f, 0.7f, 1.0f)
	};
	light_pos += 0.05f;
	opencl::buffer_object* uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																opencl::BUFFER_FLAG::INITIAL_COPY |
																opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																sizeof(oclraster_uniforms),
																(void*)&rasterize_uniforms);
	
	unsigned int argc = 0;
	//cout << "rasterizing ..." << endl;
	ocl->use_kernel("TEMPLATE_RASTERIZE");
	
	// TODO: only use necessary data (-> condense buffer in transform stage / throw away unnecessary triangles)
	ocl->set_kernel_argument(argc++, state.index_buffer); // index buffer
	ocl->set_kernel_argument(argc++, state.transformed_user_buffer); // transform output
	ocl->set_kernel_argument(argc++, uniforms_buffer);
	//
	
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, state.triangle_queues_buffer);
	ocl->set_kernel_argument(argc++, state.queue_sizes_buffer);
	ocl->set_kernel_argument(argc++, state.tile_size);
	ocl->set_kernel_argument(argc++, uint2(state.bin_count_xy));
	ocl->set_kernel_argument(argc++, (unsigned int)triangle_count); // "queue size" for now, TODO: actual one
	ocl->set_kernel_argument(argc++, const_buffer_rp);
	ocl->set_kernel_argument(argc++, state.framebuffer_size);
	ocl->set_kernel_argument(argc++, state.color_framebuffer);
	ocl->set_kernel_argument(argc++, state.depth_framebuffer);
	// TODO: use queue size? compute queue size earlier?
	ocl->set_kernel_range(ocl->compute_kernel_ranges(state.framebuffer_size.x, state.framebuffer_size.y));
	ocl->run_kernel();
	
	ocl->delete_buffer(uniforms_buffer);
}
