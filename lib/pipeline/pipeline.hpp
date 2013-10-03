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

#ifndef __OCLRASTER_PIPELINE_HPP__
#define __OCLRASTER_PIPELINE_HPP__

#include "cl/opencl.hpp"
#include "pipeline/transform_stage.hpp"
#include "pipeline/processing_stage.hpp"
#include "pipeline/binning_stage.hpp"
#include "pipeline/rasterization_stage.hpp"
#include "pipeline/image.hpp"
#include "pipeline/framebuffer.hpp"
#include "core/event.hpp"
#include "core/camera.hpp"
#include "program/oclraster_program.hpp"
#include "program/transform_program.hpp"
#include "program/rasterization_program.hpp"

// internal pipeline/draw state to handle rendering across different stages and draw calls
struct draw_state {
	union {
		struct {
			unsigned int scissor_test : 1;
			unsigned int backface_culling : 1;
			
			//
			unsigned int _unused : 30;
		};
		unsigned int flags;
	};
	
	//
	PROJECTION projection = PROJECTION::PERSPECTIVE;
	depth_state depth;
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
	const vector<framebuffer>& get_default_framebuffer() const;
	vector<framebuffer>& get_default_framebuffer();
	
	enum class DEFAULT_FRAMEBUFFER_MODE : unsigned int {
		SINGLE_BUFFERING = 1u, // only uses a single framebuffer, swap will be blocking!
		DOUBLE_BUFFERING = 2u, // uses two framebuffers in ping-pong mode, less chance of swap blocking (default mode)
		TRIPLE_BUFFERING = 3u, // uses three framebuffers, even less chance of swap blocking
		TRIPLE_BUFFERING_DISCARD = 4u, // uses three framebuffers, discards the oldest framebuffer if swap would need to block -> no blocking
	};
	// TODO: !
	//void set_default_framebuffer_mode(const DEFAULT_FRAMEBUFFER_MODE mode);
	//const DEFAULT_FRAMEBUFFER_MODE& get_default_framebuffer_mode() const;
	static constexpr size_t get_framebuffer_count_from_mode(const DEFAULT_FRAMEBUFFER_MODE mode) {
		return (mode == DEFAULT_FRAMEBUFFER_MODE::SINGLE_BUFFERING ? 1 :
				(mode == DEFAULT_FRAMEBUFFER_MODE::DOUBLE_BUFFERING ? 2 :
				 (mode == DEFAULT_FRAMEBUFFER_MODE::TRIPLE_BUFFERING ||
				  mode == DEFAULT_FRAMEBUFFER_MODE::TRIPLE_BUFFERING_DISCARD ? 3 : 1)));
	}
	
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
	
	// depth testing
	// if a custom depth function is used, this must be a function or macro defintion called "depth_test"
	// that takes the incoming fragment depth and the currently stored fragment depth and returns true
	// if the test succeeds (i.e. the fragment passes)
	// example for LESS: "#define depth_test(incoming, current) (incoming < current)"
	void set_depth_function(const DEPTH_FUNCTION depth_func,
							const string custom_depth_func = "");
	DEPTH_FUNCTION get_depth_function() const;
	const string& get_custom_depth_function() const;
	
	void set_depth_test(const bool depth_test_state);
	bool get_depth_test() const;
	
	// enable depth-override when you're writing the framebuffer depth inside a rasterization program
	// note that this is highly discouraged, since it will disable early depth testing
	void set_depth_override(const bool depth_override_state);
	bool get_depth_override() const;
	
	// set/get the complete depth state at once
	void set_depth_state(const depth_state& state);
	const depth_state& get_depth_state() const;
	
	// scissor testing
	void set_scissor_test(const bool scissor_test_state);
	bool get_scissor_test() const;
	void set_scissor_rectangle(const unsigned int& sx, const unsigned int& sy,
							   const unsigned int& swidth, const unsigned int& sheight);
	void set_scissor_rectangle(const uint2& offset, const uint2& size);
	const uint4& get_scissor_rectangle() const;
	
	//
	void _set_fxaa_state(const bool state);
	bool _get_fxaa_state() const;
	
protected:
	draw_state state;
	transform_stage transform;
	processing_stage processing;
	binning_stage binning;
	rasterization_stage rasterization;
	
	//
	void create_framebuffers(const uint2& size);
	void destroy_framebuffers();
	// array of framebuffer (size is dependent on double/triple/*-buffering)
	vector<framebuffer> default_framebuffer;
	DEFAULT_FRAMEBUFFER_MODE default_framebuffer_mode { DEFAULT_FRAMEBUFFER_MODE::DOUBLE_BUFFERING };
	size_t cur_default_fb { 0 };
	
	// fxaa
	bool fxaa_state { true };
	
	// map/copy fbo
	GLuint copy_fbo_id { 0 }, copy_fbo_tex_id { 0 };
#if defined(OCLRASTER_IOS)
	GLuint vbo_fullscreen_triangle { 0 };
#endif
	
	// camera
	camera* cam { nullptr };
	
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
	if(is_base_of<transform_program, program_type>::value) {
		state.transform_prog = (transform_program*)&program;
	}
	else if(is_base_of<rasterization_program, program_type>::value) {
		state.rasterize_prog = (rasterization_program*)&program;
	}
}

#endif
