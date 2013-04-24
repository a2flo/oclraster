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

#ifndef __OCLRASTER_PIPELINE_H__
#define __OCLRASTER_PIPELINE_H__

#include "cl/opencl.h"
#include "pipeline/transform_stage.h"
#include "pipeline/processing_stage.h"
#include "pipeline/binning_stage.h"
#include "pipeline/rasterization_stage.h"
#include "pipeline/image.h"
#include "pipeline/framebuffer.h"
#include "core/event.h"
#include "core/camera.h"
#include "program/oclraster_program.h"
#include "program/transform_program.h"
#include "program/rasterization_program.h"

// internal pipeline/draw state to handle rendering across different stages and draw calls
struct draw_state {
	// TODO: store actual flags/data
	union {
		struct {
			unsigned int depth_test : 1;
			unsigned int scissor_test : 1;
			unsigned int backface_culling : 1;
			
			//
			unsigned int _unused : 29;
		};
		unsigned int flags;
	};
	
	//
	PROJECTION projection = PROJECTION::PERSPECTIVE;
	uint4 scissor_rectangle { 0u, 0u, ~0u, ~0u };
	uint4 scissor_rectangle_abs { 0u, 0u, ~0u, ~0u }; // absolute, inclusive
	
	// NOTE: this is just for the internal transformed buffer
	const unsigned int transformed_primitive_size = 10 * sizeof(float);
	
	// framebuffers
	uint2 framebuffer_size { 1280, 720 };
	framebuffer* active_framebuffer = nullptr;
	
	//
	opencl::buffer_object* transformed_vertices_buffer = nullptr;
	opencl::buffer_object* transformed_buffer = nullptr;
	opencl::buffer_object* primitive_bounds_buffer = nullptr;
	unordered_map<string, const opencl_base::buffer_object&> user_buffers;
	unordered_map<string, const image&> user_images;
	vector<opencl::buffer_object*> user_transformed_buffers;
	
	//
	transform_program* transform_prog = nullptr;
	rasterization_program* rasterize_prog = nullptr;
	
	//
	const uint2 bin_size { OCLRASTER_BIN_SIZE };
	uint2 bin_count { 1, 1 };
	uint2 bin_offset { 0, 0 };
	const unsigned int batch_size { OCLRASTER_BATCH_SIZE };
	unsigned int batch_count { 0 };
	unsigned int primitive_count { 0 };
	unsigned int instance_primitive_count { 0 };
	unsigned int instance_index_count { 0 };
	unsigned int vertex_count { 0 };
	unsigned int instance_count { 1 };
	
	//
	struct camera_setup {
		float3 position; // actual camera position
		float3 origin; // screen origin
		float3 forward; // normalized forward vector
		float3 x_vec; // "dx", (right) vector from one screen pixel to the next (in the same screen row)
		float3 y_vec; // "dy", (up) vector from one screen pixel to the next (in the same screen column)
		// note: there is no far plane, front plane normal == -forward and normals are stored transposed
		// -> call pipeline::compute_frustum_normals instead of setting these manually
		array<float4, 3> frustum_normals;
	} cam_setup;
	opencl::buffer_object* camera_buffer = nullptr;
};

//
enum class PRIMITIVE_TYPE : unsigned int {
	TRIANGLE,
	TRIANGLE_STRIP,
	TRIANGLE_FAN
};

//
class pipeline {
public:
	pipeline();
	virtual ~pipeline();
	
	// "swaps"/displays the default framebuffer (-> blits the default framebuffer to the window framebuffer)
	void swap();
	
	// binds a transform_program or rasterization_program (or any derived class thereof)
	template <class program_type> void bind_program(const program_type& program);
	
	// buffer binding
	// NOTE: to bind the index buffer, use the name "index_buffer"
	void bind_buffer(const string& name, const opencl_base::buffer_object& buffer);
	void bind_image(const string& name, const image& img);
	void bind_framebuffer(framebuffer* fb);
	
	// any default framebuffer modification happens at your own risk!
	// attachment count and formats should never be modified
	const framebuffer* get_default_framebuffer() const;
	framebuffer* get_default_framebuffer();
	
	const framebuffer* get_bound_framebuffer() const;
	framebuffer* get_bound_framebuffer();
	
	// "draw calls", range: [first, last)
	void draw(const PRIMITIVE_TYPE type,
			  const unsigned int vertex_count,
			  const pair<unsigned int, unsigned int> element_range);
	void draw_instanced(const PRIMITIVE_TYPE type,
						const unsigned int vertex_count,
						const pair<unsigned int, unsigned int> element_range,
						const unsigned int instance_count);
	
	// camera
	// NOTE: the camera class and these functions are only provided to make things easier.
	// meaning, they don't have to be used if you don't want to use them and roll your own camera code instead.
	// in that case, the draw_state camera_setup must be set to the wanted (valid) state manually,
	// by either calling set_camera_setup_from_camera(...) with a derived camera class or directly
	// modifying the camera_setup via get_camera_setup() and manually calling update_camera_buffer()
	void set_camera(camera* cam_);
	camera* get_camera() const;
	
	// use these to manually modify the draw_state camera_setup
	const draw_state::camera_setup& get_camera_setup() const;
	draw_state::camera_setup& get_camera_setup();
	void update_camera_buffer() const;
	
	// correctly sets/computes the camera_setup from the given camera (automatically called by set_camera)
	void set_camera_setup_from_camera(camera* cam);
	
	// computes and writes the frustum normals from/to the given camera setup (automatically called by set_camera_setup_from_camera)
	void compute_frustum_normals(draw_state::camera_setup& cam_setup);
	
	// helper function to easily set the camera up for orthographic/2D rendering
	void start_orthographic_rendering();
	void stop_orthographic_rendering();
	
	// scissor testing
	void set_scissor_test(const bool scissor_test_state);
	bool get_scissor_test() const;
	void set_scissor_rectangle(const uint& sx, const uint& sy,
							   const uint& swidth, const uint& sheight);
	void set_scissor_rectangle(const uint2& offset, const uint2& size);
	const uint4& get_scissor_rectangle() const;
	
protected:
	draw_state state;
	transform_stage transform;
	processing_stage processing;
	binning_stage binning;
	rasterization_stage rasterization;
	
	//
	void create_framebuffers(const uint2& size);
	void destroy_framebuffers();
	framebuffer default_framebuffer;
	
	// map/copy fbo
	GLuint copy_fbo_id = 0, copy_fbo_tex_id = 0;
#if defined(OCLRASTER_IOS)
	GLuint vbo_fullscreen_triangle = 0;
#endif
	
	// camera
	camera* cam = nullptr;
	
	// event handler
	event::handler event_handler_fnctr;
	bool event_handler(EVENT_TYPE type, shared_ptr<event_object> obj);
	
};

//
template <class program_type> void pipeline::bind_program(const program_type& program) {
	static_assert(is_base_of<oclraster_program, program_type>::value,
				  "invalid program type (must derive from oclraster_program)!");
	static_assert(is_base_of<transform_program, program_type>::value ||
				  is_base_of<rasterization_program, program_type>::value,
				  "invalid program type (must be a transform_program or rasterization_program or a derived class)!");
	// static_if would be nice
	if(is_base_of<transform_program, program_type>::value) {
		state.transform_prog = (transform_program*)&program;
	}
	else if(is_base_of<rasterization_program, program_type>::value) {
		state.rasterize_prog = (rasterization_program*)&program;
	}
}

#endif
