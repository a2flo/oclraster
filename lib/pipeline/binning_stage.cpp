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
									  64 * 1024 * 1024); // TODO: actual size
}

binning_stage::~binning_stage() {
	if(bin_distribution_counter != nullptr) {
		ocl->delete_buffer(bin_distribution_counter);
	}
	if(queue_buffer != nullptr) {
		ocl->delete_buffer(queue_buffer);
	}
}

void binning_stage::bin(draw_state& state) {
	////
	// bin rasterizer
	
	// clear counter
	auto mapped_ptr = ocl->map_buffer(bin_distribution_counter);
	*(unsigned int*)mapped_ptr = 0;
	ocl->unmap_buffer(bin_distribution_counter, mapped_ptr);
	
	//
	unsigned int argc = 0;
	ocl->use_kernel("BIN_RASTERIZE");
	
	//
	const size_t unit_count = ocl->get_active_device()->units;
	const size2 bin_count {
		(state.framebuffer_size.x / state.tile_size.x) + ((state.framebuffer_size.x % state.tile_size.x) != 0 ? 1 : 0),
		(state.framebuffer_size.y / state.tile_size.y) + ((state.framebuffer_size.y % state.tile_size.y) != 0 ? 1 : 0)
	};
	const size_t bin_count_lin = bin_count.x * bin_count.y;
	
	const size_t bin_local_size = std::min(ocl->get_kernel_work_group_size(), bin_count_lin);
	// TODO: num_elements -> actual/remaining/batch triangle count
	
	const size_t batch_size = 256; // -> triangles indices within a batch can be stored as uchars
	const size_t batch_count = ((state.triangle_count / batch_size) +
								((state.triangle_count % batch_size) != 0 ? 1 : 0));
	
	cout << "###### binning ######" << endl;
	cout << "batch_size: " << batch_size << endl;
	cout << "batch_count: " << batch_count << endl;
	cout << "triangle_count: " << state.triangle_count << endl;
	cout << "bin_count: " << bin_count << " (" << bin_count_lin << ")" << endl;
	
	
	ocl->set_kernel_argument(argc++, bin_distribution_counter);
	ocl->set_kernel_argument(argc++, queue_buffer);
	ocl->set_kernel_argument(argc++, (uint2)bin_count);
	ocl->set_kernel_argument(argc++, (unsigned int)bin_count_lin);
	ocl->set_kernel_argument(argc++, (unsigned int)batch_count);
	ocl->set_kernel_argument(argc++, (unsigned int)batch_size);
	ocl->set_kernel_argument(argc++, (unsigned int)state.triangle_count);
	
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, state.framebuffer_size);
	
	oclr_msg("binning: %u (%u) :: %u",
			 unit_count,
			 unit_count * bin_local_size,
			 bin_local_size);
	ocl->set_kernel_range({ unit_count * bin_local_size, bin_local_size });
	ocl->run_kernel();
}
