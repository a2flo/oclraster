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

struct __attribute__((packed, aligned(16))) constant_camera_data {
	float4 camera_position;
	float4 camera_origin;
	float4 camera_x_vec;
	float4 camera_y_vec;
	float4 camera_forward;
	array<float4, 3> frustum_normals;
	uint2 viewport;
};

pipeline::pipeline() :
default_framebuffer(0, 0),
event_handler_fnctr(this, &pipeline::event_handler) {
	create_framebuffers(size2(floor::get_width(), floor::get_height()));
	state.framebuffer_size = default_framebuffer.get_size();
	state.active_framebuffer = &default_framebuffer;
	state.camera_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
											 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
											 sizeof(constant_camera_data));
	
	state.scissor_test = 0;
	state.backface_culling = 1;
	
	floor::get_event()->add_internal_event_handler(event_handler_fnctr, EVENT_TYPE::WINDOW_RESIZE, EVENT_TYPE::KERNEL_RELOAD);
	
#if defined(OCLRASTER_IOS)
	static const float fullscreen_triangle[6] { 1.0f, 1.0f, 1.0f, -3.0f, -3.0f, 1.0f };
	glGenBuffers(1, &vbo_fullscreen_triangle);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_fullscreen_triangle);
	glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(float), fullscreen_triangle, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif
}

pipeline::~pipeline() {
	floor::get_event()->remove_event_handler(event_handler_fnctr);
	
	destroy_framebuffers();
	
	ocl->delete_buffer(state.camera_buffer);
	
#if defined(OCLRASTER_IOS)
	if(glIsBuffer(vbo_fullscreen_triangle)) glDeleteBuffers(1, &vbo_fullscreen_triangle);
#endif
}

bool pipeline::event_handler(EVENT_TYPE type, shared_ptr<event_object> obj) {
	if(type == EVENT_TYPE::WINDOW_RESIZE) {
		const window_resize_event& evt = (const window_resize_event&)*obj;
		create_framebuffers(evt.size);
	}
	else if(type == EVENT_TYPE::KERNEL_RELOAD) {
		// unbind user programs, since those are invalid now
		state.transform_prog = nullptr;
		state.rasterize_prog = nullptr;
	}
	return true;
}

void pipeline::create_framebuffers(const uint2& size) {
	const bool is_default_framebuffer = (state.active_framebuffer == &default_framebuffer);
	
	// destroy old framebuffers first
	destroy_framebuffers();
	
	const uint2 scaled_size = float2(size) / floor::get_upscaling();
	log_debug("size: %v -> %v", size, scaled_size);
	
	//
	default_framebuffer = framebuffer::create_with_images(scaled_size.x, scaled_size.y,
														  { { IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA } },
														  { IMAGE_TYPE::FLOAT_32, IMAGE_CHANNEL::R });
	
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
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
	glTexImage2D(GL_TEXTURE_2D, 0,
#if !defined(OCLRASTER_IOS)
				 GL_RGBA8,
#else
				 GL_RGBA,
#endif
				 scaled_size.x, scaled_size.y,
				 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copy_fbo_tex_id, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
	
	// rebind new default framebuffer (+set correct state)
	if(is_default_framebuffer) {
		bind_framebuffer(nullptr);
	}
	
	// do an initial clear
	default_framebuffer.clear();
}

void pipeline::destroy_framebuffers() {
	framebuffer::destroy_images(default_framebuffer);
	
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
	glBindFramebuffer(GL_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
	if(copy_fbo_tex_id != 0) glDeleteTextures(1, &copy_fbo_tex_id);
	if(copy_fbo_id != 0) glDeleteFramebuffers(1, &copy_fbo_id);
	copy_fbo_tex_id = 0;
	copy_fbo_id = 0;
#endif
#endif
}

void pipeline::swap() {
	const uint2 default_fb_size = default_framebuffer.get_size();
	image* fbo_img = default_framebuffer.get_image(0);
	
#if defined(OCLRASTER_FXAA)
	if(fxaa_state) {
		// fxaa
		ocl->use_kernel("FXAA.LUMA");
		ocl->set_kernel_argument(0, fbo_img->get_buffer());
		ocl->set_kernel_argument(1, default_fb_size);
		ocl->set_kernel_range(ocl->compute_kernel_ranges(default_fb_size.x, default_fb_size.y));
		ocl->run_kernel();
		
		ocl->use_kernel("FXAA");
		ocl->set_kernel_argument(0, fbo_img->get_buffer());
		ocl->set_kernel_argument(1, default_fb_size);
		ocl->set_kernel_range(ocl->compute_kernel_ranges(default_fb_size.x, default_fb_size.y));
		ocl->run_kernel();
	}
#endif
	
	// draw/blit to screen
#if defined(OCLRASTER_IOS)
	glBindFramebuffer(GL_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
#endif
	glViewport(0, 0, floor::get_width(), floor::get_height());
	
	// copy opencl framebuffer to blit framebuffer/texture
	auto fbo_data = fbo_img->map(opencl::MAP_BUFFER_FLAG::READ | opencl::MAP_BUFFER_FLAG::BLOCK);
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
#if !defined(OCLRASTER_IOS)
	glBindFramebuffer(GL_FRAMEBUFFER, copy_fbo_id);
#endif
	glBindTexture(GL_TEXTURE_2D, copy_fbo_tex_id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, default_fb_size.x, default_fb_size.y,
					GL_RGBA, GL_UNSIGNED_BYTE, (const unsigned char*)fbo_data);
#else
	glDrawPixels(default_fb_size.x, default_fb_size.y, GL_RGBA, GL_UNSIGNED_BYTE, fbo_data);
#endif
	fbo_img->unmap(fbo_data);
	
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
#if !defined(OCLRASTER_IOS)
	// blit
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBlitFramebuffer(0, 0, default_fb_size.x, default_fb_size.y,
					  0, 0, floor::get_width(), floor::get_height(),
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
	// draw
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
#endif
	
#if !defined(OCLRASTER_IOS)
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
#endif
#endif
	
	default_framebuffer.clear();
}

void pipeline::draw(const PRIMITIVE_TYPE type,
					const unsigned int vertex_count,
					const pair<unsigned int, unsigned int> element_range) {
	draw_instanced(type, vertex_count, element_range, 1);
}

void pipeline::draw_instanced(const PRIMITIVE_TYPE type,
							  const unsigned int vertex_count,
							  const pair<unsigned int, unsigned int> element_range,
							  const unsigned int instance_count) {
	if(instance_count == 0) return;
	if(element_range.second <= element_range.first) {
		log_error("invalid element range: %u - %u", element_range.first, element_range.second);
		return;
	}
	
	if(state.scissor_test &&
	   (state.scissor_rectangle.z == 0 || state.scissor_rectangle.w == 0 ||
		state.scissor_rectangle.x >= state.framebuffer_size.x ||
		state.scissor_rectangle.y >= state.framebuffer_size.y)) {
		return; // scissor rectangle size is 0 or offset is beyond the framebuffer size
	}
	
	// initialize draw state
	state.instance_count = instance_count;
	state.instance_primitive_count = (element_range.second - element_range.first);
	state.primitive_count = state.instance_primitive_count * state.instance_count;
	state.vertex_count = vertex_count;
	switch(type) {
		case PRIMITIVE_TYPE::TRIANGLE:
			state.instance_index_count = state.instance_primitive_count * 3;
			break;
		case PRIMITIVE_TYPE::TRIANGLE_STRIP:
		case PRIMITIVE_TYPE::TRIANGLE_FAN:
			state.instance_index_count = state.instance_primitive_count + 2;
			break;
	}
	
	if(!state.scissor_test) {
		state.scissor_rectangle_abs = { 0u, 0u, ~0u, ~0u };
		state.bin_offset = { 0u, 0u };
		state.bin_count = {
			(state.framebuffer_size.x / state.bin_size.x) + ((state.framebuffer_size.x % state.bin_size.x) != 0 ? 1 : 0),
			(state.framebuffer_size.y / state.bin_size.y) + ((state.framebuffer_size.y % state.bin_size.y) != 0 ? 1 : 0)
		};
	}
	else {
		// compute absolute, inclusive and clamped scissor rectangle
		state.scissor_rectangle_abs.set(state.scissor_rectangle.x, state.scissor_rectangle.y,
										state.scissor_rectangle.x + (state.scissor_rectangle.z == 0 ? 0 : (state.scissor_rectangle.z - 1)),
										state.scissor_rectangle.y + (state.scissor_rectangle.w == 0 ? 0 : (state.scissor_rectangle.w - 1)));
		state.scissor_rectangle_abs.z = std::min(state.scissor_rectangle_abs.z, state.framebuffer_size.x - 1);
		state.scissor_rectangle_abs.w = std::min(state.scissor_rectangle_abs.w, state.framebuffer_size.y - 1);
		
		const uint2 start_bin = state.scissor_rectangle_abs.xy() / state.bin_size;
		const uint2 end_bin = state.scissor_rectangle_abs.zw() / state.bin_size;
		state.bin_count = end_bin - start_bin + 1;
		state.bin_offset = start_bin;
	}
	state.batch_count = ((state.primitive_count / OCLRASTER_BATCH_PRIMITIVE_COUNT) +
						 ((state.primitive_count % OCLRASTER_BATCH_PRIMITIVE_COUNT) != 0 ? 1 : 0));
	
	// TODO: this should be static!
	// note: internal transformed buffer size must be a multiple of "batch primitive count" primitives (necessary for the binner)
	const unsigned int pc_mod_batch_size = (state.primitive_count % OCLRASTER_BATCH_PRIMITIVE_COUNT);
	const unsigned int primitive_padding = (pc_mod_batch_size == 0 ? 0 : OCLRASTER_BATCH_PRIMITIVE_COUNT - pc_mod_batch_size);
	state.transformed_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
												  state.transformed_primitive_size * (state.primitive_count + primitive_padding));
	state.primitive_bounds_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
													   sizeof(float) * 4 * (state.primitive_count + primitive_padding));
	state.transformed_vertices_buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
														   sizeof(float) * 4 * state.vertex_count * state.instance_count);
	
	// create user transformed buffers (transform program outputs)
	const auto active_device = ocl->get_active_device();
	for(const auto& tp_struct : state.transform_prog->get_structs()) {
		if(tp_struct->type == oclraster_program::STRUCT_TYPE::OUTPUT) {
			opencl::buffer_object* buffer = ocl->create_buffer(opencl::BUFFER_FLAG::READ_WRITE,
															   // get device specific size from program
															   tp_struct->device_infos.at(active_device).struct_size * vertex_count * state.instance_count);
			state.user_transformed_buffers.push_back(buffer);
			bind_buffer(tp_struct->object_name, *buffer);
		}
	}
	
	// pipeline
	transform.transform(state);
	processing.process(state, type);
	const auto queue_buffer = binning.bin(state);
	
	// TODO: pipelining/splitting
	rasterization.rasterize(state, type, queue_buffer);
	
	//
	ocl->delete_buffer(state.transformed_buffer);
	ocl->delete_buffer(state.primitive_bounds_buffer);
	ocl->delete_buffer(state.transformed_vertices_buffer);
	
	// delete user transformed buffers
	for(const auto& ut_buffer : state.user_transformed_buffers) {
		ocl->delete_buffer(ut_buffer);
	}
	state.user_transformed_buffers.clear();
}

void pipeline::bind_buffer(const string& name, const opencl_base::buffer_object& buffer) {
	const auto existing_buffer = state.user_buffers.find(name);
	if(existing_buffer != state.user_buffers.cend()) {
		state.user_buffers.erase(existing_buffer);
	}
	state.user_buffers.emplace(name, buffer);
}

void pipeline::bind_image(const string& name, const image& img) {
	const auto existing_image = state.user_images.find(name);
	if(existing_image != state.user_images.cend()) {
		state.user_images.erase(existing_image);
	}
	state.user_images.emplace(name, img);
}

void pipeline::bind_framebuffer(framebuffer* fb) {
	if(fb == nullptr) {
		state.active_framebuffer = &default_framebuffer;
	}
	else state.active_framebuffer = fb;
	
	//
	const auto new_fb_size = state.active_framebuffer->get_size();
	if(new_fb_size.x != state.framebuffer_size.x ||
	   new_fb_size.y != state.framebuffer_size.y) {
		state.framebuffer_size = new_fb_size;
		
		// the orthographic rendering depends on the framebuffer size
		// -> if it changes, the camera setup must be recomputed
		if(state.projection == PROJECTION::ORTHOGRAPHIC) {
			start_orthographic_rendering();
		}
	}
}

const framebuffer* pipeline::get_default_framebuffer() const {
	return &default_framebuffer;
}

framebuffer* pipeline::get_default_framebuffer() {
	return &default_framebuffer;
}

const framebuffer* pipeline::get_bound_framebuffer() const {
	return state.active_framebuffer;
}

framebuffer* pipeline::get_bound_framebuffer() {
	return state.active_framebuffer;
}

void pipeline::set_camera(camera* cam_) {
	cam = cam_;
	set_camera_setup_from_camera(cam);
}

camera* pipeline::get_camera() const {
	return cam;
}

void pipeline::set_camera_setup_from_camera(camera* cam_) {
	if(cam_ == nullptr) return;
	
	const float2 fp_framebuffer_size { (float)state.framebuffer_size.x, (float)state.framebuffer_size.y };
	const float aspect_ratio = fp_framebuffer_size.x / fp_framebuffer_size.y;
	const float angle_ratio = tanf(DEG2RAD(floor::get_fov() * 0.5f)) * 2.0f;
	
	const float3 forward_(cam_->get_forward());
	const float3 up_(cam_->get_up());
	
	const float3 right { (up_ ^ forward_).normalized() };
	const float3 up { (forward_ ^ right).normalized() };
	const float3 forward { forward_.normalized() };
	//log_debug("right: %v, up: %v, forward: %v", right, up, forward);
	
	const float3 width_vec { right * angle_ratio * aspect_ratio };
	const float3 height_vec { up * angle_ratio };
	const float3 half_width_vec { width_vec * 0.5f };
	const float3 half_height_vec { height_vec * 0.5f };
	//log_debug("w/h: %v %v, %f %f", width_vec, height_vec, aspect_ratio, angle_ratio);
	
	state.cam_setup.position = cam_->get_position();
	state.cam_setup.x_vec = width_vec / fp_framebuffer_size.x;
	state.cam_setup.y_vec = height_vec / fp_framebuffer_size.y;
	state.cam_setup.origin = forward - half_width_vec - half_height_vec;
	state.cam_setup.origin.normalized();
	state.cam_setup.forward = forward;
	
	compute_frustum_normals(state.cam_setup);
	
	// update const camera buffer
	update_camera_buffer();
}

void pipeline::update_camera_buffer() const {
	const constant_camera_data cam_data {
		state.cam_setup.position,
		state.cam_setup.origin,
		state.cam_setup.x_vec,
		state.cam_setup.y_vec,
		state.cam_setup.forward,
		state.cam_setup.frustum_normals,
		state.framebuffer_size
	};
	ocl->write_buffer(state.camera_buffer, &cam_data);
}

void pipeline::compute_frustum_normals(draw_state::camera_setup& cam_setup) {
	const float2 fp_framebuffer_size { (float)state.framebuffer_size.x, (float)state.framebuffer_size.y };
	const float3 half_width_vec = cam_setup.x_vec * (fp_framebuffer_size.x * 0.5f);
	const float3 half_height_vec = cam_setup.y_vec * (fp_framebuffer_size.y * 0.5f);
	
	// compute frustum (normals of left/top/right/bottom plane)
	// note: near plane normal == forward vector, and since there is no
	// far plane, this doesn't have to computed either (would be -forward)
	const array<float3, 4> near_points {
		{
			cam_setup.origin, // bottom left
			cam_setup.forward - half_width_vec + half_height_vec, // top left
			cam_setup.forward + half_width_vec + half_height_vec, // top right
			cam_setup.forward + half_width_vec - half_height_vec, // bottom right
		}
	};
	
	// to compute the outer planes normals, we don't need to compute "far points"
	// since they are located on the same ray as the "near points" which also
	// always goes through (0, 0, 0)
	// -> pretend the near points are the far points and (0, 0, 0) is the near
	// point in all cases (substraction is omitted for obvious reasons)
	// also note: normals point inwards for algorithmic reasons
	const array<float3, 4> frustum_normals = {
		{
			(near_points[1] ^ near_points[0]).normalized(), // left
			(near_points[2] ^ near_points[1]).normalized(), // top
			(near_points[3] ^ near_points[2]).normalized(), // right
			(near_points[0] ^ near_points[3]).normalized(), // bottom
		}
	};
	// "transpose" to save memory and do 4 computations at a time (if possible)
	for(size_t i = 0; i < 3; i++) {
		cam_setup.frustum_normals[i].x = frustum_normals[0][i];
		cam_setup.frustum_normals[i].y = frustum_normals[1][i];
		cam_setup.frustum_normals[i].z = frustum_normals[2][i];
		cam_setup.frustum_normals[i].w = frustum_normals[3][i];
	}
}

const draw_state::camera_setup& pipeline::get_camera_setup() const {
	return state.cam_setup;
}

draw_state::camera_setup& pipeline::get_camera_setup() {
	return state.cam_setup;
}

void pipeline::start_orthographic_rendering() {
	state.projection = PROJECTION::ORTHOGRAPHIC;
	const float2 fp_framebuffer_size { (float)state.framebuffer_size.x, (float)state.framebuffer_size.y };
	const float aspect_ratio = fp_framebuffer_size.x / fp_framebuffer_size.y;
	state.cam_setup.position.set(0.0f, 0.0f, 0.0f);
	state.cam_setup.origin.set(0.0f, 0.0f, 0.0f);
	state.cam_setup.x_vec.set(aspect_ratio / (float)state.framebuffer_size.x, 0.0f, 0.0f);
	state.cam_setup.y_vec.set(0.0f, 1.0f / (float)state.framebuffer_size.y, 0.0f);
	state.cam_setup.forward.set(0.0f, 0.0f, 1.0f);
	compute_frustum_normals(state.cam_setup);
	update_camera_buffer();
}

void pipeline::stop_orthographic_rendering() {
	state.projection = PROJECTION::PERSPECTIVE;
	set_camera_setup_from_camera(cam);
}

void pipeline::set_scissor_test(const bool scissor_test_state) {
	state.scissor_test = scissor_test_state;
}

bool pipeline::get_scissor_test() const {
	return state.scissor_test;
}

void pipeline::set_scissor_rectangle(const unsigned int& sx, const unsigned int& sy,
									 const unsigned int& swidth, const unsigned int& sheight) {
	state.scissor_rectangle.set(sx, sy, swidth, sheight);
}

void pipeline::set_scissor_rectangle(const uint2& offset, const uint2& size) {
	state.scissor_rectangle.set(offset.x, offset.y, size.x, size.y);
}

const uint4& pipeline::get_scissor_rectangle() const {
	return state.scissor_rectangle;
}

void pipeline::set_depth_function(const DEPTH_FUNCTION depth_func,
								  const string custom_depth_func) {
	state.depth.depth_func = depth_func;
	state.depth.custom_depth_func = (depth_func == DEPTH_FUNCTION::CUSTOM ? custom_depth_func : "");
}

DEPTH_FUNCTION pipeline::get_depth_function() const {
	return state.depth.depth_func;
}

const string& pipeline::get_custom_depth_function() const {
	return state.depth.custom_depth_func;
}

void pipeline::set_depth_test(const bool depth_test_state) {
	state.depth.depth_test = depth_test_state;
}

bool pipeline::get_depth_test() const {
	return state.depth.depth_test;
}

void pipeline::set_depth_override(const bool depth_override_state) {
	state.depth.depth_override = depth_override_state;
}

bool pipeline::get_depth_override() const {
	return state.depth.depth_override;
}

void pipeline::set_depth_state(const depth_state& dstate) {
	state.depth = dstate;
}

const depth_state& pipeline::get_depth_state() const {
	return state.depth;
}

void pipeline::_set_fxaa_state(const bool state_) {
	fxaa_state = state_;
}

bool pipeline::_get_fxaa_state() const {
	return fxaa_state;
}
