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

pipeline::pipeline() : ocl(oclraster::get_opencl()) {
	framebuffer = rtt::add_buffer(1280, 720, GL_TEXTURE_2D, TEXTURE_FILTERING::POINT, rtt::TEXTURE_ANTI_ALIASING::NONE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, 1, rtt::DEPTH_TYPE::NONE);
	framebuffer_cl = ocl->create_ogl_image2d_buffer(opencl::BUFFER_FLAG::WRITE, framebuffer->tex[0]);
}

pipeline::~pipeline() {
	if(framebuffer_cl != nullptr) ocl->delete_buffer(framebuffer_cl);
	if(framebuffer != nullptr) rtt::delete_buffer(framebuffer);
	
	if(triangle_queues_buffer != nullptr) ocl->delete_buffer(triangle_queues_buffer);
	if(queue_sizes_buffer != nullptr) ocl->delete_buffer(queue_sizes_buffer);
	if(triangle_queues_buffer_zero != nullptr) delete [] triangle_queues_buffer_zero;
	if(queue_sizes_buffer_zero != nullptr) delete [] queue_sizes_buffer_zero;
}

void pipeline::draw(const transform_stage::vertex_buffer& vb,
					const transform_stage::index_buffer& ib,
					//const transform_stage::constant_buffer& cb,
					const draw_flags& flags,
					const pair<unsigned int, unsigned int> element_range) {
#if defined(OCLRASTER_DEBUG)
	// TODO: check buffer sanity/correctness (in debug mode?)
#endif
	
	// TODO: remove this
	if(triangle_queues_buffer == nullptr ||
	   queue_sizes_buffer == nullptr ||
	   triangle_queues_buffer_zero == nullptr ||
	   queue_sizes_buffer_zero == nullptr) {
		oclr_error("queue buffers are uninitialized!");
		return;
	}
	
	const uint2 screen_size(oclraster::get_width(), oclraster::get_height()); // TODO: upscaling support
	
	////
	// transform
	const auto& cam = oclraster::get_camera_setup();
	struct __attribute__((packed)) constant_data_tp {
		float4 camera_position;
		float4 camera_origin;
		float4 camera_x_vec;
		float4 camera_y_vec;
	} const_data_tp {
		cam.position,
		cam.origin,
		cam.x_vec,
		cam.y_vec,
	};
	opencl_base::buffer_object* const_buffer_tp = ocl->create_buffer(opencl::BUFFER_FLAG::READ | opencl::BUFFER_FLAG::INITIAL_COPY, sizeof(constant_data_tp), &const_data_tp);
	
	const auto num_elements = (element_range.first != ~0u ?
							   element_range.second - element_range.first + 1 :
							   ib.index_count / 3);
	opencl_base::buffer_object* transformed_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
																		flags.transformed_primitive_size * num_elements);

	// -> 1D kernel, with max #work-items per work-group
	// -> index_range / (max #work-items per work-group)
	ocl->use_kernel("TRANSFORM");
	unsigned int argc = 0;
	ocl->set_kernel_argument(argc++, vb.buffer);
	ocl->set_kernel_argument(argc++, ib.buffer);
	ocl->set_kernel_argument(argc++, transformed_buffer);
	ocl->set_kernel_argument(argc++, const_buffer_tp);
	ocl->set_kernel_argument(argc++, num_elements);
	const size_t wg_size = ocl->get_kernel_work_group_size();
	const size_t transform_range = (num_elements / wg_size) * wg_size + wg_size; // next size divisible by wg_size
	//cout << "TRANSFORM: " << wg_size << ", " << transform_range << ", " << num_elements << endl;
	ocl->set_kernel_range(cl::NDRange(transform_range), cl::NDRange(wg_size));
	ocl->run_kernel();
	//ocl->finish();
	
	ocl->delete_buffer(const_buffer_tp);
	
	////
	// bin rasterizer
	/*const size2 tile_size = (ocl->get_active_device()->type == opencl::DEVICE_TYPE::CPU0 ?
							 size2(1, 1) : size2(16, 16)); // TODO: appropriate/configurable size
	const size2 is_oversized = screen_size % tile_size;
	const size2 bin_global_range = (screen_size / tile_size) + size2(is_oversized.x != 0 ? 1 : 0,
																	 is_oversized.y != 0 ? 1 : 0);
	cout << "bins: " << bin_global_range << " (tile size: " << tile_size << ")" << endl;*/
	
	const uint2 bin_count_xy(screen_size.x / tile_size.x + ((screen_size.x % tile_size.x) != 0 ? 1 : 0),
							 screen_size.y / tile_size.y + ((screen_size.y % tile_size.y) != 0 ? 1 : 0));
	const size_t bin_count = bin_count_xy.x * bin_count_xy.y;
	
	//
	ocl->use_kernel("BIN_RASTERIZE");
	
	// zero out queues
	//ocl->write_buffer(triangle_queues_buffer, triangle_queues_buffer_zero); // no need to clear this, since it'll overwritten
	ocl->write_buffer(queue_sizes_buffer, queue_sizes_buffer_zero); // this however must be cleared!
	// TODO: if opencl 1.2 is available -> use clear buffer command?
	
	const size_t bin_local_size = ocl->get_kernel_work_group_size();
	// TODO: num_elements -> actual/remaining/batch triangle count
	const size_t bin_global_size = ((num_elements / bin_local_size) +
									((num_elements % bin_local_size) != 0 ? 1 : 0));
	const size_t triangles_per_group = ((num_elements / bin_global_size) +
										((num_elements % bin_global_size) != 0 ? 1 : 0));
	
	cout << "#####" << endl;
	cout << "bin_local_size: " << bin_local_size << endl;
	cout << "bin_global_size: " << bin_global_size << endl;
	cout << "triangles_per_group: " << triangles_per_group << endl;
	cout << "#triangles: " << num_elements << endl;
	cout << "bin_count: " << bin_count << ", " << bin_count_xy << endl;
	cout << "triangle_queues_buffer size: " << (sizeof(unsigned int) * num_elements * bin_count) << endl;
	
	argc = 0;
	ocl->set_kernel_argument(argc++, transformed_buffer);
	ocl->set_kernel_argument(argc++, triangle_queues_buffer);
	ocl->set_kernel_argument(argc++, queue_sizes_buffer);
	ocl->set_kernel_argument(argc++, screen_size);
	ocl->set_kernel_argument(argc++, uint2(tile_size));
	ocl->set_kernel_argument(argc++, uint2(bin_count_xy));
	ocl->set_kernel_argument(argc++, (unsigned int)triangles_per_group);
	ocl->set_kernel_argument(argc++, (unsigned int)num_elements); // TODO: remove this, compute actual count in transform kernel and pass-through to bin rasterizer
	ocl->set_kernel_range(cl::NDRange(bin_global_size * bin_local_size),
						  cl::NDRange(bin_local_size));
	ocl->run_kernel();
	
	////
	// render / rasterization
	// TODO: workaround? clear doesn't work reliably ...
	/*rtt::start_draw(framebuffer);
	rtt::clear();
	rtt::stop_draw();*/
	argc = 0;
	ocl->use_kernel("CLEAR_FRAMEBUFFER");
	ocl->set_kernel_argument(argc++, framebuffer_cl);
	ocl->set_kernel_range(cl::NDRange(screen_size.x, screen_size.y),
						  ocl->get_active_device()->type == opencl::DEVICE_TYPE::CPU0 ?
						  cl::NDRange(1, 1) :
						  cl::NDRange(16, 16)); // TODO: better size
	ocl->run_kernel();
	
	struct __attribute__((packed)) constant_data_rp {
		const unsigned int triangle_count;
	} const_data_rp {
		num_elements
	};
	opencl_base::buffer_object* const_buffer_rp = ocl->create_buffer(opencl::BUFFER_FLAG::READ | opencl::BUFFER_FLAG::INITIAL_COPY, sizeof(constant_data_rp), &const_data_rp);
	
	// TODO: clear framebuffer(s)?
	argc = 0;
	ocl->use_kernel("RASTERIZE");
	//cout << "raster wg: " << ocl->get_kernel_work_group_size() << endl;
	ocl->set_kernel_argument(argc++, transformed_buffer);
	ocl->set_kernel_argument(argc++, triangle_queues_buffer);
	ocl->set_kernel_argument(argc++, queue_sizes_buffer);
	ocl->set_kernel_argument(argc++, tile_size);
	ocl->set_kernel_argument(argc++, uint2(bin_count_xy));
	ocl->set_kernel_argument(argc++, (unsigned int)num_elements); // "queue size" for now, TODO: actual one
	ocl->set_kernel_argument(argc++, const_buffer_rp);
	ocl->set_kernel_argument(argc++, framebuffer_cl);
	ocl->set_kernel_range(cl::NDRange(screen_size.x, screen_size.y),
						  ocl->get_active_device()->type == opencl::DEVICE_TYPE::CPU0 ?
						  cl::NDRange(1, 1) :
						  cl::NDRange(16, 16));
	ocl->run_kernel();
	
	// draw/blit to screen
	oclraster::start_2d_draw();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer->fbo_id);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, screen_size.x, screen_size.y,
					  0, 0, screen_size.x, screen_size.y,
					  //0, 0, screen_size.x*2, screen_size.y*2,
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
	oclraster::stop_2d_draw();
	
	ocl->delete_buffer(const_buffer_rp);
	
	//
	ocl->delete_buffer(transformed_buffer);
}

void pipeline::reserve_memory(const unsigned int triangle_count) {
	reserved_triangle_count = triangle_count;
	
	// NOTE: this must be called again if the screen size changes! (TODO: ?)
	const uint2 screen_size(oclraster::get_width(), oclraster::get_height());
	oclr_msg("initial screen size: %v", screen_size);
	const uint2 bin_count_xy(screen_size.x / tile_size.x + ((screen_size.x % tile_size.x) != 0 ? 1 : 0),
							 screen_size.y / tile_size.y + ((screen_size.y % tile_size.y) != 0 ? 1 : 0));
	const size_t bin_count = bin_count_xy.x * bin_count_xy.y;
	
	triangle_queues_buffer_zero = new unsigned int[triangle_count * bin_count];
	queue_sizes_buffer_zero = new unsigned int[bin_count];
	memset(triangle_queues_buffer_zero, 0, sizeof(unsigned int) * triangle_count * bin_count); // zero init is necessary
	memset(queue_sizes_buffer_zero, 0, sizeof(unsigned int) * bin_count);
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
