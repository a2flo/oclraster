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

#include "pipeline.h"
#include "oclraster.h"

#if defined(OCLRASTER_IOS)
#include "ios_helper.h"
#endif

struct __attribute__((packed, aligned(16))) info_buffer_struct {
	unsigned int passing_triangles;
};

pipeline::pipeline() :
window_handler(this, &pipeline::window_event_handler) {
	create_framebuffers(size2(oclraster::get_width(), oclraster::get_height()));
	oclraster::get_event()->add_internal_event_handler(window_handler, EVENT_TYPE::WINDOW_RESIZE);

	info_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
									 opencl::BUFFER_FLAG::BLOCK_ON_READ |
									 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
									 sizeof(info_buffer_struct));
	
#if defined(OCLRASTER_IOS)
	static const float fullscreen_triangle[6] { 1.0f, 1.0f, 1.0f, -3.0f, -3.0f, 1.0f };
	glGenBuffers(1, &vbo_fullscreen_triangle);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_fullscreen_triangle);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), fullscreen_triangle, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

pipeline::~pipeline() {
	oclraster::get_event()->remove_event_handler(window_handler);
	
	destroy_framebuffers();
	if(triangle_queues_buffer != nullptr) ocl->delete_buffer(triangle_queues_buffer);
	if(queue_sizes_buffer != nullptr) ocl->delete_buffer(queue_sizes_buffer);
	if(triangle_queues_buffer_zero != nullptr) delete [] triangle_queues_buffer_zero;
	if(queue_sizes_buffer_zero != nullptr) delete [] queue_sizes_buffer_zero;
	
	if(info_buffer != nullptr) {
		ocl->delete_buffer(info_buffer);
	}
	
#if defined(OCLRASTER_IOS)
	if(glIsBuffer(vbo_fullscreen_triangle)) glDeleteBuffers(1, &vbo_fullscreen_triangle);
#endif
}

bool pipeline::window_event_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::WINDOW_RESIZE) {
		const window_resize_event& evt = (const window_resize_event&)*obj;
		create_framebuffers(evt.size);
	}
	return true;
}

void pipeline::create_framebuffers(const uint2& size) {
	// destroy old framebuffers first
	destroy_framebuffers();
	
	const uint2 scaled_size = float2(size) / oclraster::get_upscaling();
	framebuffer_size = scaled_size;
	oclr_debug("size: %v -> %v", size, scaled_size);
	
	//
	color_framebuffer_cl = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
											  opencl::BUFFER_FLAG::BLOCK_ON_READ |
											  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											  framebuffer_size.x * framebuffer_size.y * 4); // uchar4
	depth_framebuffer_cl = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
											  opencl::BUFFER_FLAG::BLOCK_ON_READ |
											  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											  framebuffer_size.x * framebuffer_size.y * sizeof(float));
	
	// create a fbo for copying the color framebuffer every frame and displaying it
	// (there is no other way, unfortunately)
	glGenFramebuffers(1, &copy_fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, copy_fbo_id);
	glGenTextures(1, &copy_fbo_tex_id);
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, rtt::convert_internal_format(GL_RGBA8), framebuffer_size.x, framebuffer_size.y,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copy_fbo_tex_id, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	//
	_reserve_memory(reserved_triangle_count);
}

void pipeline::destroy_framebuffers() {
	if(color_framebuffer_cl != nullptr) ocl->delete_buffer(color_framebuffer_cl);
	if(depth_framebuffer_cl != nullptr) ocl->delete_buffer(depth_framebuffer_cl);
	color_framebuffer_cl = nullptr;
	depth_framebuffer_cl = nullptr;
	
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
	if(copy_fbo_tex_id != 0) glDeleteTextures(1, &copy_fbo_tex_id);
	if(copy_fbo_id != 0) glDeleteFramebuffers(1, &copy_fbo_id);
	copy_fbo_tex_id = 0;
	copy_fbo_id = 0;
}

void pipeline::start() {
	// TODO: remove this
	if(triangle_queues_buffer == nullptr ||
	   queue_sizes_buffer == nullptr ||
	   triangle_queues_buffer_zero == nullptr ||
	   queue_sizes_buffer_zero == nullptr) {
		oclr_error("queue buffers are uninitialized!");
		return;
	}
	
	// clear framebuffer
	unsigned int argc = 0;
	ocl->use_kernel("CLEAR_COLOR_DEPTH_FRAMEBUFFER");
	ocl->set_kernel_argument(argc++, framebuffer_size);
	ocl->set_kernel_argument(argc++, color_framebuffer_cl);
	ocl->set_kernel_argument(argc++, depth_framebuffer_cl);
	ocl->set_kernel_range(ocl->compute_kernel_ranges(framebuffer_size.x, framebuffer_size.y));
	ocl->run_kernel();
}

void pipeline::stop() {
	// draw/blit to screen
#if !defined(OCLRASTER_IOS)
	oclraster::start_2d_draw();
	
	void* fbo_data = ocl->map_buffer(color_framebuffer_cl, opencl::MAP_BUFFER_FLAG::READ | opencl::MAP_BUFFER_FLAG::BLOCK);
	glBindFramebuffer(GL_FRAMEBUFFER, copy_fbo_id);
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer_size.x, framebuffer_size.y,
					GL_RGBA, GL_UNSIGNED_BYTE, fbo_data);
	ocl->unmap_buffer(color_framebuffer_cl, fbo_data);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	glBlitFramebuffer(0, 0, framebuffer_size.x, framebuffer_size.y,
					  0, 0, oclraster::get_width(), oclraster::get_height(),
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	oclraster::stop_2d_draw();
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
#else
	glBindFramebuffer(GL_FRAMEBUFFER, OCLRASTER_DEFAULT_FRAMEBUFFER);
	oclraster::start_2d_draw();
	
	void* fbo_data = ocl->map_buffer(color_framebuffer_cl, opencl::BUFFER_FLAG::READ, true);
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, framebuffer_size.x, framebuffer_size.y,
					GL_RGBA, GL_UNSIGNED_BYTE, fbo_data);
	ocl->unmap_buffer(color_framebuffer_cl, fbo_data);
	
	//
	shader_object* shd = ios_helper::get_shader("BLIT");
	glUseProgram(shd->program.program);
	glUniform1i(shd->program.uniforms.find("tex")->second.location, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	
	glFrontFace(GL_CW);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_fullscreen_triangle);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glFrontFace(GL_CCW);
	
	glUseProgram(0);
	
	oclraster::stop_2d_draw();
#endif
}

void pipeline::draw(const pair<unsigned int, unsigned int> element_range) {
#if defined(OCLRASTER_DEBUG)
	// TODO: check buffer sanity/correctness (in debug mode)
#endif
	
	// initialize draw state
	state.depth_test = 1;
	state.transformed_primitive_size = 16 * sizeof(float); // NOTE: this is just for the internal transformed buffer
	state.framebuffer_size = framebuffer_size;
	state.color_framebuffer = color_framebuffer_cl;
	state.depth_framebuffer = depth_framebuffer_cl;
	//state.tile_size = tile_size; // TODO: dynamic?
	state.triangle_queues_buffer = triangle_queues_buffer;
	state.queue_sizes_buffer = queue_sizes_buffer;
	state.triangle_queues_buffer_zero = triangle_queues_buffer_zero;
	state.queue_sizes_buffer_zero = queue_sizes_buffer_zero;
	state.reserved_triangle_count = reserved_triangle_count;
	state.info_buffer = info_buffer;
	
	const auto index_count = (element_range.second - element_range.first + 1) * 3;
	const auto num_elements = element_range.second - element_range.first + 1;
	/*const auto num_elements = (element_range.first != ~0u ?
							   element_range.second - element_range.first + 1 :
							   ib.index_count / 3);*/
	
	// TODO: this should be static!
	state.transformed_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
												  state.transformed_primitive_size * num_elements);
	
	// clear info buffer
	const info_buffer_struct empty_info_buffer { 0 };
	ocl->write_buffer(info_buffer, &empty_info_buffer);
	
	// create user transformed buffers (transform program outputs)
	const auto active_device = ocl->get_active_device();
	for(const auto& tp_struct : state.transform_prog->get_structs()) {
		if(tp_struct.type == oclraster_program::STRUCT_TYPE::OUTPUT) {
			opencl::buffer_object* buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
															   // get device specific size from program
															   tp_struct.device_infos.at(active_device).struct_size * index_count);
			state.user_transformed_buffers.push_back(buffer);
			bind_buffer(tp_struct.object_name, *buffer);
		}
	}
	
	// pipeline
	transform.transform(state, num_elements);
	const unsigned int post_transform_triangle_count = binning.bin(state);
	if(post_transform_triangle_count > 0) {
		// TODO: pipelining/splitting
		// TODO: actual triangle count (queue size?)
		rasterization.rasterize(state, post_transform_triangle_count);
	}
	
	//
	ocl->delete_buffer(state.transformed_buffer);
	
	// delete user transformed buffers
	for(const auto& ut_buffer : state.user_transformed_buffers) {
		ocl->delete_buffer(ut_buffer);
	}
	state.user_transformed_buffers.clear();
}

void pipeline::_reserve_memory(const unsigned int triangle_count) {
	reserved_triangle_count = triangle_count;
	
	// NOTE: this must be called again if the screen size changes!
	const uint2 bin_count_xy(framebuffer_size.x / tile_size.x + ((framebuffer_size.x % tile_size.x) != 0 ? 1 : 0),
							 framebuffer_size.y / tile_size.y + ((framebuffer_size.y % tile_size.y) != 0 ? 1 : 0));
	const size_t bin_count = bin_count_xy.x * bin_count_xy.y;
	
	triangle_queues_buffer_zero = new unsigned int[triangle_count * bin_count];
	queue_sizes_buffer_zero = new unsigned int[bin_count];
	memset(triangle_queues_buffer_zero, 0, sizeof(unsigned int) * triangle_count * bin_count); // zero init is necessary
	memset(queue_sizes_buffer_zero, 0, sizeof(unsigned int) * bin_count);
	// TODO: delete old buffers
	triangle_queues_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
												opencl::BUFFER_FLAG::INITIAL_COPY |
												opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
												sizeof(unsigned int) * triangle_count * bin_count,
												triangle_queues_buffer_zero);
	queue_sizes_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE |
											opencl::BUFFER_FLAG::INITIAL_COPY |
											opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											sizeof(unsigned int) * bin_count,
											queue_sizes_buffer_zero);
}

void pipeline::bind_buffer(const string& name, const opencl_base::buffer_object& buffer) {
	const auto existing_buffer = state.user_buffers.find(name);
	if(existing_buffer != state.user_buffers.cend()) {
		// TODO: unbind previously bound buffer
		state.user_buffers.erase(existing_buffer);
	}
	state.user_buffers.emplace(name, buffer);
}

void pipeline::bind_image(const string& name, const image& img) {
	bind_buffer(name, *img.get_buffer()); // TODO: let the user directly call this?
}
