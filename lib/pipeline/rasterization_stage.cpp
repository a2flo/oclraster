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
	bin_distribution_counter = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
												  opencl::BUFFER_FLAG::BLOCK_ON_READ |
												  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
												  sizeof(unsigned int));
}

rasterization_stage::~rasterization_stage() {
	if(bin_distribution_counter != nullptr) {
		ocl->delete_buffer(bin_distribution_counter);
	}
}

void rasterization_stage::rasterize(draw_state& state,
									const PRIMITIVE_TYPE type,
									const opencl_base::buffer_object* queue_buffer) {
	////
	// render / rasterization
	oclraster_program::kernel_image_spec image_spec;
	if(!create_kernel_image_spec(state, *state.rasterize_prog, image_spec)) {
		return;
	}
	ocl->use_kernel(state.rasterize_prog->get_kernel(image_spec));
	
	// determine per-bin work-group size and how many iterations/splits are necessary per bin
	const size_t bin_size = state.bin_size.x * state.bin_size.y;
	size_t wg_size = ocl->get_kernel_work_group_size();;
	if(ocl->get_active_device()->type >= opencl::DEVICE_TYPE::CPU0 &&
	   ocl->get_active_device()->type <= opencl::DEVICE_TYPE::CPU255) {
		// for whatever reason, using a work-group size of 1 runs a lot faster than using 128 (most cpu implementations)
		wg_size = 1;
	}
	
	const size_t local_size = std::min(wg_size, bin_size);
	const size_t intra_bin_groups = (bin_size / local_size) + (bin_size % local_size != 0 ? 1 : 0);
	
	//
	unsigned int argc = 0;
	if(!bind_user_buffers(state, *state.rasterize_prog, argc)) return;
	
	const auto index_buffer = state.user_buffers.find("index_buffer");
	if(index_buffer == state.user_buffers.cend()) {
		oclr_error("index buffer not bound!");
		return;
	}
	ocl->set_kernel_argument(argc++, &index_buffer->second);
	
	ocl->set_kernel_argument(argc++, bin_distribution_counter);
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, queue_buffer);
	ocl->set_kernel_argument(argc++, state.bin_count);
	ocl->set_kernel_argument(argc++, (unsigned int)(state.bin_count.x * state.bin_count.y));
	ocl->set_kernel_argument(argc++, state.batch_count);
	ocl->set_kernel_argument(argc++, intra_bin_groups);
	ocl->set_kernel_argument(argc++, (underlying_type<PRIMITIVE_TYPE>::type)type);
	ocl->set_kernel_argument(argc++, state.framebuffer_size);
	
	if(ocl->get_active_device()->type >= opencl::DEVICE_TYPE::CPU0 &&
	   ocl->get_active_device()->type <= opencl::DEVICE_TYPE::CPU255) {
		// cpu
		const size_t bin_count_lin = state.bin_count.x * state.bin_count.y;
		ocl->set_kernel_range({ bin_count_lin * wg_size, wg_size });
	}
	else {
		// gpu
		const size_t unit_count = ocl->get_active_device()->units;
		ocl->set_kernel_range({ unit_count * local_size, local_size });
	}
	ocl->run_kernel();
}
