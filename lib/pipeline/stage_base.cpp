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
		const auto buffer = state.user_buffers.find(user_struct->object_name);
		// TODO: only check this in debug mode?
		if(buffer == state.user_buffers.cend()) {
			oclr_error("buffer \"%s\" not bound!", user_struct->object_name);
			return false;
		}
		ocl->set_kernel_argument(argc++, &buffer->second);
	}
	
	// TODO: merge this with create_kernel_spec ...?
	const framebuffer* fb = state.active_framebuffer;
	const auto images = program.get_images();
	for(size_t i = 0, fb_img_idx = 0, img_count = images.image_names.size(); i < img_count; i++) {
		if(images.is_framebuffer[i]) {
			// framebuffer
			if(fb == nullptr) {
				oclr_error("no framebuffer is currently bound!");
				return false;
			}
			const image* img = nullptr;
			switch(images.image_types[i]) {
				case oclraster_program::IMAGE_VAR_TYPE::DEPTH_IMAGE:
					img = fb->get_depth_buffer();
					break;
				case oclraster_program::IMAGE_VAR_TYPE::STENCIL_IMAGE:
					img = fb->get_stencil_buffer();
					break;
				default:
					img = fb->get_image(fb_img_idx);
					fb_img_idx++;
					break;
			}
			if(img == nullptr) {
				oclr_error("framebuffer image \"%s\" not bound!", images.image_names[i]);
				return false;
			}
			ocl->set_kernel_argument(argc++, img->get_buffer());
		}
		else {
			// image
			const auto& img_name = images.image_names[i];
			const auto img = state.user_images.find(img_name);
			// TODO: only check this in debug mode?
			if(img == state.user_images.cend()) {
				oclr_error("image \"%s\" not bound!", img_name);
				return false;
			}
			ocl->set_kernel_argument(argc++, img->second.get_buffer());
		}
	}
	
	return true;
}

bool stage_base::create_kernel_spec(const draw_state& state, const oclraster_program& program,
									oclraster_program::kernel_spec& spec) {
	const auto images = program.get_images();
	const framebuffer* fb = state.active_framebuffer;
	for(size_t i = 0, fb_img_idx = 0, img_count = images.image_names.size(); i < img_count; i++) {
		if(images.is_framebuffer[i]) {
			// framebuffer
			if(fb == nullptr) {
				oclr_error("no framebuffer is currently bound!");
				return false;
			}
			const image* img = nullptr;
			switch(images.image_types[i]) {
				case oclraster_program::IMAGE_VAR_TYPE::DEPTH_IMAGE:
					img = fb->get_depth_buffer();
					break;
				case oclraster_program::IMAGE_VAR_TYPE::STENCIL_IMAGE:
					img = fb->get_stencil_buffer();
					break;
				default:
					img = fb->get_image(fb_img_idx);
					fb_img_idx++;
					break;
			}
			if(img == nullptr) {
				oclr_error("framebuffer image \"%s\" not bound!", images.image_names[i]);
				return false;
			}
			spec.image_spec.emplace_back(img->get_image_type());
		}
		else {
			// image
			const auto& img_name = images.image_names[i];
			const auto img = state.user_images.find(img_name);
			if(img == state.user_images.cend()) {
				oclr_error("image \"%s\" not bound!", img_name);
				return false;
			}
			spec.image_spec.emplace_back(img->second.get_image_type());
		}
	}
	spec.projection = state.projection;
	return true;
}
