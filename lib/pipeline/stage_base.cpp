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

#include "stage_base.h"
#include "pipeline.h"
#include "oclraster.h"

stage_base::stage_base() {
}

stage_base::~stage_base() {
}

bool stage_base::bind_user_buffers(const draw_state& state, const oclraster_program& program, unsigned int& argc) {
	// set user buffers
	for(const auto& user_struct : program.get_structs()) {
		const auto buffer = state.user_buffers.find(user_struct.object_name);
		// TODO: only check this in debug mode?
		if(buffer == state.user_buffers.cend()) {
			oclr_error("buffer \"%s\" not bound!", user_struct.object_name);
			return false;
		}
		ocl->set_kernel_argument(argc++, &buffer->second);
	}
	for(const auto& img_name : program.get_images().image_names) {
		const auto img = state.user_images.find(img_name);
		// TODO: only check this in debug mode?
		if(img == state.user_images.cend()) {
			oclr_error("image \"%s\" not bound!", img_name);
			return false;
		}
		ocl->set_kernel_argument(argc++, img->second.get_buffer());
	}
	return true;
}

bool stage_base::create_kernel_image_spec(const draw_state& state, const oclraster_program& program,
										  oclraster_program::kernel_image_spec& image_spec) {
	const auto images = program.get_images();
	for(const auto& img_name : images.image_names) {
		const auto img = state.user_images.find(img_name);
		if(img == state.user_images.cend()) {
			oclr_error("image \"%s\" not bound!", img_name);
			return false;
		}
		image_spec.emplace_back(make_image_type(img->second.get_data_type(), img->second.get_channel_order()));
	}
	return true;
}
