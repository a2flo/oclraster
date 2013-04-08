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

#include "transform_stage.h"
#include "pipeline.h"
#include "oclraster.h"

struct __attribute__((packed, aligned(16))) constant_data_tp {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
	float4 camera_forward;
	array<float4, 3> frustum_normals;
	uint2 viewport;
};

transform_stage::transform_stage() : stage_base() {
	const_buffer_tp = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
										 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
										 sizeof(constant_data_tp));
}

transform_stage::~transform_stage() {
	ocl->delete_buffer(const_buffer_tp);
}

void transform_stage::transform(draw_state& state, const unsigned int& num_elements) {
	// update const buffer
	const constant_data_tp const_data_tp {
		state.cam_setup.position,
		state.cam_setup.origin,
		state.cam_setup.x_vec,
		state.cam_setup.y_vec,
		state.cam_setup.forward,
		state.cam_setup.frustum_normals,
		state.framebuffer_size
	};
	ocl->write_buffer(const_buffer_tp, &const_data_tp); // TODO: make this non-blocking
	
	//
	oclraster_program::kernel_image_spec image_spec;
	if(!create_kernel_image_spec(state, *state.transform_prog, image_spec)) {
		return;
	}
	
	// -> 1D kernel, with max #work-items per work-group
	// -> index_range / (max #work-items per work-group)
	ocl->use_kernel(state.transform_prog->get_kernel(image_spec));
	
	unsigned int argc = 0;
	if(!bind_user_buffers(state, *state.transform_prog, argc)) return;
	
	const auto index_buffer = state.user_buffers.find("index_buffer");
	if(index_buffer == state.user_buffers.cend()) {
		oclr_error("index buffer not bound!");
		return;
	}
	ocl->set_kernel_argument(argc++, &index_buffer->second);
	
	// internal buffer / kernel parameters
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, state.triangle_bounds_buffer);
	ocl->set_kernel_argument(argc++, const_buffer_tp);
	ocl->set_kernel_argument(argc++, num_elements);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(num_elements));
	ocl->run_kernel();
}
