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

rasterization_stage::rasterization_stage() : stage_base() {
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
	oclraster_program::kernel_image_spec image_spec;
	if(!create_kernel_image_spec(state, *state.rasterize_prog, image_spec)) {
		return;
	}
	ocl->use_kernel(state.rasterize_prog->get_kernel(image_spec));
	
	//
	unsigned int argc = 0;
	if(!bind_user_buffers(state, *state.rasterize_prog, argc)) return;
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
