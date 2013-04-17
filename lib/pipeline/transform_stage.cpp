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

transform_stage::transform_stage() : stage_base() {
}

transform_stage::~transform_stage() {
}

void transform_stage::transform(draw_state& state, const unsigned int& vertex_count) {
	//
	oclraster_program::kernel_image_spec image_spec;
	if(!create_kernel_image_spec(state, *state.transform_prog, image_spec)) {
		return;
	}
	
	// -> 1D kernel, with max #work-items per work-group
	ocl->use_kernel(state.transform_prog->get_kernel(image_spec));
	
	unsigned int argc = 0;
	if(!bind_user_buffers(state, *state.transform_prog, argc)) return;
	
	// internal buffer / kernel parameters
	ocl->set_kernel_argument(argc++, state.transformed_vertices_buffer);
	ocl->set_kernel_argument(argc++, state.camera_buffer);
	ocl->set_kernel_argument(argc++, vertex_count);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(vertex_count));
	ocl->run_kernel();
}
