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

void transform_stage::transform(draw_state& state) {
	//
	oclraster_program::kernel_spec spec;
	if(!create_kernel_spec(state, *state.transform_prog, spec)) {
		return;
	}
	
	// -> 1D kernel, with max #work-items per work-group
	ocl->use_kernel(state.transform_prog->get_kernel(spec));
	
	unsigned int argc = 0;
	if(!bind_user_buffers(state, *state.transform_prog, argc)) return;
	
	// internal buffer / kernel parameters
	ocl->set_kernel_argument(argc++, state.transformed_vertices_buffer);
	ocl->set_kernel_argument(argc++, state.camera_buffer);
	ocl->set_kernel_argument(argc++, state.vertex_count);
	ocl->set_kernel_argument(argc++, state.instance_count);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(state.vertex_count * state.instance_count));
	ocl->run_kernel();
	
	//
#if 0
	static bool dumped = false;
	if(!dumped) {
		dumped = true;
		ocl->dump_buffer(state.transformed_vertices_buffer, oclraster::data_path("dump/tvbuffer.bin"));
		
		//
		unsigned int buffer_num = 0;
		const auto find_and_dump_buffer = [&](const string& name) -> void {
			const auto buffer = state.user_buffers.find(name);
			ocl->dump_buffer((opencl::buffer_object*)&buffer->second, oclraster::data_path("dump/tuser_"+uint2string(buffer_num)+".hex"));
			buffer_num++;
		};
		
		for(const auto& user_struct : state.transform_prog->get_structs()) {
			if(user_struct->type != oclraster_program::STRUCT_TYPE::BUFFERS) {
				find_and_dump_buffer(user_struct->object_name);
			}
			else {
				for(size_t i = 0, buffer_entries = user_struct->variables.size(); i < buffer_entries; i++) {
					find_and_dump_buffer(user_struct->variables[i]);
				}
			}
		}
	}
#endif
}
