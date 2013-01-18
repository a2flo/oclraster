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
#if defined(__APPLE__)
#include "osx_helper.h"
#endif
#include "a2m.h" // TODO: remove this again

pipeline::pipeline() :
window_handler(this, &pipeline::window_event_handler) {
	create_framebuffers(size2(oclraster::get_width(), oclraster::get_height()));
	oclraster::get_event()->add_internal_event_handler(window_handler, EVENT_TYPE::WINDOW_RESIZE);
}

pipeline::~pipeline() {
	oclraster::get_event()->remove_event_handler(window_handler);
	
	if(triangle_queues_buffer != nullptr) ocl->delete_buffer(triangle_queues_buffer);
	if(queue_sizes_buffer != nullptr) ocl->delete_buffer(queue_sizes_buffer);
	if(triangle_queues_buffer_zero != nullptr) delete [] triangle_queues_buffer_zero;
	if(queue_sizes_buffer_zero != nullptr) delete [] queue_sizes_buffer_zero;
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
	
	color_framebuffer = rtt::add_buffer(scaled_size.x, scaled_size.y, GL_TEXTURE_2D, TEXTURE_FILTERING::POINT, rtt::TEXTURE_ANTI_ALIASING::NONE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 1, rtt::DEPTH_TYPE::NONE);
	color_framebuffer_cl = ocl->create_ogl_image2d_buffer(opencl::BUFFER_FLAG::READ_WRITE, color_framebuffer->tex[0]);
	
	// shared float texture doesn't work on the cpu, cl float image2d doesn't work on the gpu ... -> use the correct one
	// TODO: correct device type check
	if(ocl->get_active_device()->type == opencl::DEVICE_TYPE::GPU0) {
		depth_framebuffer = rtt::add_buffer(scaled_size.x, scaled_size.y, GL_TEXTURE_2D, TEXTURE_FILTERING::POINT, rtt::TEXTURE_ANTI_ALIASING::NONE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_R32F, GL_RED, GL_FLOAT, 1, rtt::DEPTH_TYPE::NONE);
		depth_framebuffer_cl = ocl->create_ogl_image2d_buffer(opencl::BUFFER_FLAG::READ_WRITE, depth_framebuffer->tex[0]);
	}
	else {
		depth_framebuffer_cl = ocl->create_image2d_buffer(opencl::BUFFER_FLAG::READ_WRITE,
														  // CL_Rx is supported on all cl platforms except for AMD
														  ocl->get_platform_vendor() != opencl::PLATFORM_VENDOR::AMD ? CL_Rx : CL_R,
														  CL_FLOAT, scaled_size.x, scaled_size.y);
	}
	
	//
	_reserve_memory(reserved_triangle_count);
}

void pipeline::destroy_framebuffers() {
	if(color_framebuffer_cl != nullptr) ocl->delete_buffer(color_framebuffer_cl);
	if(color_framebuffer != nullptr) rtt::delete_buffer(color_framebuffer);
	if(depth_framebuffer_cl != nullptr) ocl->delete_buffer(depth_framebuffer_cl);
	if(depth_framebuffer != nullptr) rtt::delete_buffer(depth_framebuffer);
	color_framebuffer = nullptr;
	color_framebuffer_cl = nullptr;
	depth_framebuffer = nullptr;
	depth_framebuffer_cl = nullptr;
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
	// TODO: decide which to use
	/*rtt::start_draw(color_framebuffer);
	rtt::clear();
	rtt::stop_draw();
	rtt::start_draw(depth_framebuffer);
	rtt::clear();
	rtt::stop_draw();*/
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
	if(ocl->get_active_device()->type == opencl::DEVICE_TYPE::CPU0) {
		ocl->flush();
		ocl->finish(); // finish before blitting if this is a cpu device
	}
	oclraster::start_2d_draw();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, color_framebuffer->fbo_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, framebuffer_size.x, framebuffer_size.y,
					  0, 0, oclraster::get_width(), oclraster::get_height(),
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	oclraster::stop_2d_draw();
}

void pipeline::draw(const transform_stage::vertex_buffer& vb,
					const transform_stage::index_buffer& ib,
					//const transform_stage::constant_buffer& cb,
					const pair<unsigned int, unsigned int> element_range) {
#if defined(OCLRASTER_DEBUG)
	// TODO: check buffer sanity/correctness (in debug mode)
#endif
	
	// initialize draw state
	state.depth_test = 1;
	state.transformed_primitive_size = 4 * sizeof(float4); // TODO: get from vertex buffer?
	state.framebuffer_size = framebuffer_size;
	state.color_framebuffer = color_framebuffer_cl;
	state.depth_framebuffer = depth_framebuffer_cl;
	//state.tile_size = tile_size; // TODO: dynamic?
	state.triangle_queues_buffer = triangle_queues_buffer;
	state.queue_sizes_buffer = queue_sizes_buffer;
	state.triangle_queues_buffer_zero = triangle_queues_buffer_zero;
	state.queue_sizes_buffer_zero = queue_sizes_buffer_zero;
	state.reserved_triangle_count = reserved_triangle_count;
	
	const auto num_elements = (element_range.first != ~0u ?
							   element_range.second - element_range.first + 1 :
							   ib.index_count / 3);
	
	state.transformed_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
												  state.transformed_primitive_size * num_elements);
	state.transformed_user_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
													   // TODO: get size from program
													   sizeof(a2m::vertex_data) * ib.index_count);
	state.index_buffer = ib.buffer;
	
	// pipeline
	transform.transform(state, vb, ib, num_elements);
	binning.bin(state, num_elements); // TODO: actual triangle count and pipelining/splitting
	rasterization.rasterize(state, num_elements); // TODO: actual triangle count (queue size?)
	
	//
	ocl->delete_buffer(state.transformed_buffer);
	ocl->delete_buffer(state.transformed_user_buffer);
}

void pipeline::_reserve_memory(const unsigned int triangle_count) {
	reserved_triangle_count = triangle_count;
	
	// NOTE: this must be called again if the screen size changes! (TODO: ?)
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
