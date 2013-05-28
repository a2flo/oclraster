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

#include "binning_stage.h"
#include "pipeline.h"
#include "oclraster.h"

binning_stage::binning_stage() {
	bin_distribution_counter = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
												  opencl::BUFFER_FLAG::BLOCK_ON_READ |
												  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
												  sizeof(unsigned int));
	queue_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
									  opencl::BUFFER_FLAG::BLOCK_ON_READ |
									  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
									  128 * 1024 * 1024); // TODO: actual size
}

binning_stage::~binning_stage() {
	if(bin_distribution_counter != nullptr) {
		ocl->delete_buffer(bin_distribution_counter);
	}
	if(queue_buffer != nullptr) {
		ocl->delete_buffer(queue_buffer);
	}
}

const opencl::buffer_object* binning_stage::bin(draw_state& state) {
	////
	// bin rasterizer
	unsigned int argc = 0;
	ocl->use_kernel("BIN_RASTERIZE");
	
	//
	const size_t unit_count = ocl->get_active_device()->units;
	const size_t bin_count_lin = state.bin_count.x * state.bin_count.y;
	
	const size_t wg_size = ocl->get_kernel_work_group_size();
	const size_t bin_local_size = std::min(wg_size, bin_count_lin);
	
	ocl->set_kernel_argument(argc++, bin_distribution_counter);
	ocl->set_kernel_argument(argc++, queue_buffer);
	ocl->set_kernel_argument(argc++, (uint2)state.bin_count);
	ocl->set_kernel_argument(argc++, (unsigned int)bin_count_lin);
	ocl->set_kernel_argument(argc++, state.bin_offset);
	ocl->set_kernel_argument(argc++, state.batch_count);
	ocl->set_kernel_argument(argc++, state.primitive_count);
	
	ocl->set_kernel_argument(argc++, state.primitive_bounds_buffer);
	ocl->set_kernel_argument(argc++, state.framebuffer_size);
	
	if(ocl->get_active_device()->type >= opencl::DEVICE_TYPE::CPU0 &&
	   ocl->get_active_device()->type <= opencl::DEVICE_TYPE::CPU255) {
		// cpu
		ocl->set_kernel_range({ bin_count_lin * wg_size, wg_size });
	}
	else {
		// gpu
		const size_t intra_bin_groups = (bin_count_lin / bin_local_size) + (bin_count_lin % bin_local_size != 0 ? 1 : 0);
		ocl->set_kernel_argument(argc++, (unsigned int)intra_bin_groups);
		ocl->set_kernel_range({ unit_count * bin_local_size, bin_local_size });
	}
	ocl->run_kernel();
	
	return queue_buffer;
}
