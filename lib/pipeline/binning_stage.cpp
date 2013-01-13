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
}

binning_stage::~binning_stage() {
}

void binning_stage::bin(draw_state& state,
						const unsigned int& triangle_count) {
	////
	// bin rasterizer
	/*const size2 tile_size = (ocl->get_active_device()->type == opencl::DEVICE_TYPE::CPU0 ?
	 size2(1, 1) : size2(16, 16)); // TODO: appropriate/configurable size
	 const size2 is_oversized = framebuffer_size % tile_size;
	 const size2 bin_global_range = (framebuffer_size / tile_size) + size2(is_oversized.x != 0 ? 1 : 0,
	 is_oversized.y != 0 ? 1 : 0);
	 cout << "bins: " << bin_global_range << " (tile size: " << tile_size << ")" << endl;*/
	
	const uint2 bin_count_xy(state.framebuffer_size.x / state.tile_size.x + ((state.framebuffer_size.x % state.tile_size.x) != 0 ? 1 : 0),
							 state.framebuffer_size.y / state.tile_size.y + ((state.framebuffer_size.y % state.tile_size.y) != 0 ? 1 : 0));
	const unsigned int bin_count = bin_count_xy.x * bin_count_xy.y;
	state.bin_count_xy = bin_count_xy;
	state.bin_count = bin_count;
	
	//
	unsigned int argc = 0;
	ocl->use_kernel("BIN_RASTERIZE");
	
	// zero out queues
	//ocl->write_buffer(triangle_queues_buffer, triangle_queues_buffer_zero); // no need to clear this, since it'll overwritten
	ocl->write_buffer(state.queue_sizes_buffer, state.queue_sizes_buffer_zero); // this however must be cleared!
	// TODO: if opencl 1.2 is available -> use clear buffer command?
	
	const size_t bin_local_size = ocl->get_kernel_work_group_size();
	// TODO: num_elements -> actual/remaining/batch triangle count
	const size_t bin_global_size = ((triangle_count / bin_local_size) +
									((triangle_count % bin_local_size) != 0 ? 1 : 0));
	const size_t triangles_per_group = ((triangle_count / bin_global_size) +
										((triangle_count % bin_global_size) != 0 ? 1 : 0));
	
	/*cout << "#####" << endl;
	 cout << "bin_local_size: " << bin_local_size << endl;
	 cout << "bin_global_size: " << bin_global_size << endl;
	 cout << "triangles_per_group: " << triangles_per_group << endl;
	 cout << "#triangles: " << num_elements << endl;
	 cout << "bin_count: " << bin_count << ", " << bin_count_xy << endl;
	 cout << "triangle_queues_buffer size: " << (sizeof(unsigned int) * num_elements * bin_count) << endl;*/
	
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, state.triangle_queues_buffer);
	ocl->set_kernel_argument(argc++, state.queue_sizes_buffer);
	ocl->set_kernel_argument(argc++, state.framebuffer_size);
	ocl->set_kernel_argument(argc++, uint2(state.tile_size));
	ocl->set_kernel_argument(argc++, uint2(bin_count_xy));
	ocl->set_kernel_argument(argc++, (unsigned int)triangles_per_group);
	ocl->set_kernel_argument(argc++, (unsigned int)triangle_count); // TODO: remove this, compute actual count in transform kernel and pass-through to bin rasterizer
	//cout << "binning: " << (bin_global_size * bin_local_size) << " -> " << bin_local_size << endl;
	ocl->set_kernel_range({
		cl::NDRange(bin_global_size * bin_local_size),
		cl::NDRange(bin_local_size)});
	ocl->run_kernel();
}
