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

#include "processing_stage.h"
#include "pipeline.h"
#include "oclraster.h"

processing_stage::processing_stage() : stage_base() {
}

processing_stage::~processing_stage() {
}

void processing_stage::process(draw_state& state, const PRIMITIVE_TYPE type) {
	// -> 1D kernel, with max #work-items per work-group
	ocl->use_kernel(string("PROCESSING.") + (state.projection == PROJECTION::PERSPECTIVE ? "PERSPECTIVE" : "ORTHOGRAPHIC"));
	
	unsigned int argc = 0;
	
	const auto index_buffer = state.user_buffers.find("index_buffer");
	if(index_buffer == state.user_buffers.cend()) {
		log_error("index buffer not bound!");
		return;
	}
	ocl->set_kernel_argument(argc++, &index_buffer->second);
	
	// internal buffer / kernel parameters
	ocl->set_kernel_argument(argc++, state.transformed_vertices_buffer);
	ocl->set_kernel_argument(argc++, state.transformed_buffer);
	ocl->set_kernel_argument(argc++, state.primitive_bounds_buffer);
	ocl->set_kernel_argument(argc++, state.camera_buffer);
	ocl->set_kernel_argument(argc++, (underlying_type<PRIMITIVE_TYPE>::type)type);
	ocl->set_kernel_argument(argc++, state.primitive_count);
	ocl->set_kernel_argument(argc++, state.instance_primitive_count);
	ocl->set_kernel_argument(argc++, state.instance_index_count);
	ocl->set_kernel_argument(argc++, state.scissor_rectangle_abs);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(state.primitive_count));
	ocl->run_kernel();
}
