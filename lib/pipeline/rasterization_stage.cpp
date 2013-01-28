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
	unsigned int argc = 0;
	ocl->use_kernel(state.rasterize_prog->get_identifier());
	
	// TODO: only use necessary data (-> condense buffer in transform stage / throw away unnecessary triangles)
	// set user buffers
	for(const auto& user_struct : state.rasterize_prog->get_structs()) {
		const auto buffer = state.user_buffers.find(user_struct.object_name);
		// TODO: only check this in debug mode?
		if(buffer == state.user_buffers.cend()) {
			oclr_error("buffer \"%s\" not bound!", user_struct.object_name);
			return;
		}
		ocl->set_kernel_argument(argc++, &buffer->second);
	}
	
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
}
