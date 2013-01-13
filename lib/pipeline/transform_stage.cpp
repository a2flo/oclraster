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

struct __attribute__((packed)) constant_data_tp {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
	float4 camera_forward;
	uint2 viewport;
};

transform_stage::transform_stage() {
	const_buffer_tp = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
										 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
										 sizeof(constant_data_tp));
	
	_debug_buffer.fill(0);
	/*_debug_buffer_tp = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
										  opencl::BUFFER_FLAG::INITIAL_COPY |
										  opencl::BUFFER_FLAG::READ_BACK_RESULT |
										  opencl::BUFFER_FLAG::BLOCK_ON_READ |
										  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
										  _debug_buffer_size, &_debug_buffer[0]);*/
}

transform_stage::~transform_stage() {
	ocl->delete_buffer(const_buffer_tp);
	//ocl->delete_buffer(_debug_buffer_tp);
}

void transform_stage::transform(draw_state& state,
								const transform_stage::vertex_buffer& vb,
								const transform_stage::index_buffer& ib,
								const unsigned int& num_elements) {
	// update const buffer
	const auto& cam = oclraster::get_camera_setup();
	const constant_data_tp const_data_tp {
		cam.position,
		cam.origin,
		cam.x_vec,
		cam.y_vec,
		cam.forward,
		state.framebuffer_size
	};
	ocl->write_buffer(const_buffer_tp, &const_data_tp); // TODO: make this non-blocking
	
	// -> 1D kernel, with max #work-items per work-group
	// -> index_range / (max #work-items per work-group)
	unsigned int argc = 0;

	struct __attribute__((packed, aligned(16))) simple_output {
		float4 vertex;
		float4 normal;
		float2 tex_coord;
	};
	
	const auto identity_matrix = matrix4f();
	opencl::buffer_object* uniforms_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
																opencl::BUFFER_FLAG::INITIAL_COPY |
																opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
																sizeof(matrix4f),
																(void*)&identity_matrix);
	
	ocl->use_kernel("TEMPLATE_TRANSFORM");
	ocl->set_kernel_argument(argc++, vb.buffer);
	ocl->set_kernel_argument(argc++, state.transformed_user_buffer);
	ocl->set_kernel_argument(argc++, uniforms_buffer);
	
	ocl->set_kernel_argument(argc++, ib.buffer);
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, const_buffer_tp);
	ocl->set_kernel_argument(argc++, num_elements);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(num_elements));
	ocl->run_kernel();
	
	ocl->delete_buffer(uniforms_buffer);
	
#if 0
	// debug buffer results
	const int dbg_value_count = _debug_buffer[0];
	oclr_debug("#debug values: %d", dbg_value_count);
	for(int i = 0; i < dbg_value_count; i++) {
		oclr_debug("\t%d: %d", i, _debug_buffer[i+1]);
	}
#endif
}
