/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
 *  Copyright (C) 2004 - 2013 Florian Ziesche
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

#ifndef __OCLRASTER_SUPPORT_GFX2D_H__
#define __OCLRASTER_SUPPORT_GFX2D_H__

#include <oclraster_support/global.h>
#include <oclraster/oclraster.h>
#include <oclraster/core/core.h>
#include <oclraster/core/event.h>
#include <oclraster/cl/opencl.h>
#include <oclraster/pipeline/pipeline.h>
#include <oclraster/program/transform_program.h>
#include <oclraster/program/rasterization_program.h>
#include "rendering/shader.h"

#define __GFX2D_POINT_COMPUTE_FUNCS(F, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_point, point, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_line, line, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_triangle, triangle, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_rectangle, rectangle, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_rounded_rectangle, rounded_rectangle, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_circle, circle, DS_FUNC, DS_NAME) \
F(gfx2d::point_compute_circle_sector, circle_sector, DS_FUNC, DS_NAME)

#define __GFX2D_DRAW_STYLE_FUNCS(PF, F) \
F(PF, gfx2d::draw_style_fill, fill) \
F(PF, gfx2d::draw_style_gradient, gradient) \
F(PF, gfx2d::draw_style_texture, texture) \
F(PF, gfx2d::draw_style_border<gfx2d::draw_style_fill>, border_fill) \
F(PF, gfx2d::draw_style_border<gfx2d::draw_style_gradient>, border_gradient) \
F(PF, gfx2d::draw_style_border<gfx2d::draw_style_texture>, border_texture)

#define __GFX2D_DEFINE_DRAW_FUNC(pc_func, pc_name, ds_func, ds_name) \
template<typename... Args> static void draw_ ##pc_name ##_ ##ds_name(const Args&... args) { \
	draw<pc_func<ds_func>>(args...); \
}

class OCLRASTER_API gfx2d {
public:
	//
	gfx2d() = delete;
	~gfx2d() = delete;
	static void init(pipeline* p);
	static void destroy();
	
	//
	enum class GRADIENT_TYPE : unsigned int {
		HORIZONTAL,
		VERTICAL,
		DIAGONAL_LR,
		DIAGONAL_RL
	};
	
	enum class BLEND_MODE : unsigned int {
		DEFAULT,
		ADD,
		PRE_MUL,
		COLOR,
		ALPHA
	};
	
	enum class CORNER : unsigned int {
		NONE			= 0,
		TOP_RIGHT		= (1 << 0),
		BOTTOM_RIGHT	= (1 << 1),
		BOTTOM_LEFT		= (1 << 2),
		TOP_LEFT		= (1 << 3),
		ALL				= (TOP_RIGHT | BOTTOM_RIGHT | BOTTOM_LEFT | TOP_LEFT)
	};
	enum_class_bitwise_or(CORNER)
	
	// draw functions
	template <class point_compute_draw_spec, typename... Args>
	static void draw(const Args&... args) {
		point_compute_draw_spec::compute_and_draw(args...);
	}
	
	template <class draw_style> struct point_compute_point;
	template <class draw_style> struct point_compute_line;
	template <class draw_style> struct point_compute_triangle;
	template <class draw_style> struct point_compute_rectangle;
	template <class draw_style> struct point_compute_rounded_rectangle;
	template <class draw_style> struct point_compute_circle;
	template <class draw_style> struct point_compute_circle_sector;
	
	struct draw_style_fill;
	struct draw_style_gradient;
	struct draw_style_texture;
	template <class draw_style_next> struct draw_style_border;
	
	// some macro voodoo for user convenience (e.g. draw_rectangle_gradient(...))
	__GFX2D_DRAW_STYLE_FUNCS(__GFX2D_DEFINE_DRAW_FUNC, __GFX2D_POINT_COMPUTE_FUNCS)
	
	struct primitive_properties {
		oclraster_struct primitive_point {
			// this struct is necessary for correct alignment
			float2 vertex;
			constexpr primitive_point(float2&& p) noexcept : vertex(p) {}
			constexpr primitive_point(const float2& p) noexcept : vertex(p) {}
			constexpr primitive_point(const float& x, const float& y) noexcept : vertex(x, y) {}
			primitive_point& operator=(const float2& p) {
				vertex = p;
				return *this;
			}
			primitive_point& operator=(float2&& p) {
				vertex = p;
				return *this;
			}
		};
		vector<primitive_point> points;
		float4 extent;
		PRIMITIVE_TYPE primitive_type;
		union {
			struct {
				// specifies if the primitive has a mid point (e.g. circle, rounded rect)
				unsigned int has_mid_point : 1;
				unsigned int has_equal_start_end_point : 1;
				unsigned int border_connect_start_end_point : 1;
				unsigned int border_swap_strip_points : 1;
				
				//
				unsigned int _unused : 28;
			};
			unsigned int flags = 0;
		};
		primitive_properties(const PRIMITIVE_TYPE& primitive_type_) :
		points(), extent(), primitive_type(primitive_type_) {}
		primitive_properties() :
		points(), extent(), primitive_type(PRIMITIVE_TYPE::TRIANGLE) {}
		primitive_properties& operator=(primitive_properties&& props) {
			points.swap(props.points);
			extent = props.extent;
			primitive_type = props.primitive_type;
			flags = props.flags;
			return *this;
		}
	};
	
	// helper functions
	static bool is_pnt_in_rectangle(const rect& rectangle, const pnt& point);
	static bool is_pnt_in_rectangle(const rect& rectangle, const ipnt& point);

	static void compute_ellipsoid_points(vector<primitive_properties::primitive_point>& dst_points,
										 const float& radius_lr, const float& radius_tb,
										 const float& start_angle, const float& end_angle);
	
protected:
	static pipeline* oclr_pipeline;
			
	static opencl::buffer_object* primitives_buffer;
	static opencl::buffer_object* primitives_indices;
	
	static void upload_points_and_draw(const primitive_properties& props);
	
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*
 *	// point compute interface:
 *  template <class draw_style>
 *	struct point_compute_interface {
 *		template<typename... Args> static void compute_and_draw(...args...);
 *	};
 */

template <class draw_style>
struct gfx2d::point_compute_point {
	template<typename... Args> static void compute_and_draw(const float2& p,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_STRIP);
		props.points = {
			float2(p.x, p.y + 1.0f),
			float2(p.x, p.y),
			float2(p.x + 1.0f, p.y + 1.0f),
			float2(p.x + 1.0f, p.y)
		};
		props.extent.set(p.x, p.y, p.x + 1.0f, p.y + 1.0f);
		draw_style::draw(props, args...);
	}
};
template <class draw_style>
struct gfx2d::point_compute_line {
	template<typename... Args> static void compute_and_draw(const float2& start_pnt,
															const float2& end_pnt,
															const Args&... args) {
		compute_and_draw(start_pnt, end_pnt, 1.0f, args...);
	}
	template<typename... Args> static void compute_and_draw(const float2& start_pnt,
															const float2& end_pnt,
															const float& thickness,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_STRIP);
		
		const float half_thickness = thickness * 0.5f;
		float x1 = start_pnt.x, x2 = end_pnt.x, y1 = start_pnt.y, y2 = end_pnt.y;
		
		// add half a pixel if this is a horizontal/vertical line - this is necessary to get
		// sharp lines (anti-aliasing would 50/50 distribute the color to two pixels otherwise)
		if(x1 == x2) {
			x1 += 0.5f;
			x2 += 0.5f;
		}
		else if(y1 == y2) {
			y1 += 0.5f;
			y2 += 0.5f;
		}
		// else: diagonal
		
		// swap points if first point is below second point
		if(y2 < y1) {
			swap(x1, x2);
			swap(y1, y2);
		}
		
		// compute line direction and rotate by 90° to the left + right and multiply
		// by the half thickness while we're at it to get the correct offset
		const float2 line_dir(float2(x2 - x1, y2 - y1).normalize() * half_thickness);
		const float2 offset_rot_left(-line_dir.y, line_dir.x);
		const float2 offset_rot_right(line_dir.y, -line_dir.x);
		
		props.points.emplace_back(x2 + offset_rot_right.x, y2 + offset_rot_right.y);
		props.points.emplace_back(x2 + offset_rot_left.x, y2 + offset_rot_left.y);
		props.points.emplace_back(x1 + offset_rot_right.x, y1 + offset_rot_right.y);
		props.points.emplace_back(x1 + offset_rot_left.x, y1 + offset_rot_left.y);
		
		//
		props.extent.set(std::min(start_pnt.x, end_pnt.x), std::min(start_pnt.y, end_pnt.y),
						 std::max(start_pnt.x, end_pnt.x), std::max(start_pnt.y, end_pnt.y));
		draw_style::draw(props, args...);
	}
};
template <class draw_style>
struct gfx2d::point_compute_triangle {
	template<typename... Args> static void compute_and_draw(const float2& p0,
															const float2& p1,
															const float2& p2,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE);
		props.points = { p0, p1, p2 };
		props.extent.set(std::min(p0.x, std::min(p1.x, p2.x)), std::min(p0.y, std::min(p1.y, p2.y)),
						 std::max(p0.x, std::min(p1.x, p2.x)), std::max(p0.y, std::min(p1.y, p2.y)));
		draw_style::draw(props, args...);
	}
};
template <class draw_style>
struct gfx2d::point_compute_rectangle {
	template<typename... Args> static void compute_and_draw(const rect& r,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_STRIP);
		props.points = {
			// 1 0 2 --- 1 2 3
			float2(r.x1, r.y2),
			float2(r.x1, r.y1),
			float2(r.x2, r.y2),
			float2(r.x2, r.y1),
		};
		props.extent.set(std::min(r.x1, r.x2), std::min(r.y1, r.y2),
						 std::max(r.x1, r.x2), std::max(r.y1, r.y2));
		props.border_connect_start_end_point = 1;
		props.border_swap_strip_points = 1;
		draw_style::draw(props, args...);
	}
	template<typename... Args> static void compute_and_draw(const float4& r,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_STRIP);
		props.points = {
			// 1 0 2 --- 1 2 3
			float2(r.x, r.w),
			float2(r.x, r.y),
			float2(r.z, r.w),
			float2(r.z, r.y),
		};
		props.extent.set(std::min(r.x, r.z), std::min(r.y, r.w),
						 std::max(r.x, r.z), std::max(r.y, r.w));
		props.border_connect_start_end_point = 1;
		props.border_swap_strip_points = 1;
		draw_style::draw(props, args...);
	}
};
template <class draw_style>
struct gfx2d::point_compute_rounded_rectangle {
	template<typename... Args> static void compute_and_draw(const rect& r,
															const float& radius,
															const CORNER corners,
															const Args&... args) {
		compute_and_draw(float4 { (float)r.x1, (float)r.y1, (float)r.x2, (float)r.y2 }, radius, corners, args...);
	}
	
	template<typename... Args> static void compute_and_draw(const float4& r,
															const float& radius,
															const CORNER corners,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_FAN);
		props.has_mid_point = 1;
		
		// just in case ...
		if(corners == CORNER::NONE) {
			point_compute_rectangle<draw_style>::compute_and_draw(r, args...);
			return;
		}
		
		// start off with the mid point
		const float2 mid_point((r.x + r.z) * 0.5f,
							   (r.y + r.w) * 0.5f);
		props.points.emplace_back(mid_point);
		
		// 0: rt, 90: rb, 180: lb, 270: lt
		for(ssize_t i = 0; i < 4; i++) {
			float2 corner_point(i < 2 ? r.z : r.x, (i == 0 || i == 3) ? r.w : r.y);
			if((unsigned int)corners & (1 << i)) {
				// if this is a rounded corner, add 90° circle sector for that corner
				const size_t cur_size = props.points.size();
				gfx2d::compute_ellipsoid_points(props.points, radius, radius,
												float(i) * 90.0f, float(i+1) * 90.0f);
				
				corner_point.x += (i < 2 ? -radius : radius);
				corner_point.y += (i == 0 || i == 3 ? -radius : radius);
				for(size_t j = cur_size; j < props.points.size(); j++) {
					props.points[j].vertex.x += corner_point.x;
					props.points[j].vertex.y += corner_point.y;
				}
			}
			else {
				// else: just add the corner point
				props.points.emplace_back(corner_point);
			}
		}
		
		// add first outer point so we have a complete "circle"
		props.points.emplace_back(props.points[1]);
		props.has_equal_start_end_point = 1;
		
		//
		props.extent.set(std::min(r.x, r.z), std::min(r.y, r.w),
						 std::max(r.x, r.z), std::max(r.y, r.w));
		
		draw_style::draw(props, args...);
		
	}
};
template <class draw_style>
struct gfx2d::point_compute_circle {
	template<typename... Args> static void compute_and_draw(const float2& p,
															const float radius_lr,
															const float radius_tb,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_FAN);
		props.has_mid_point = 1;
		props.points.emplace_back(p);
		
		gfx2d::compute_ellipsoid_points(props.points, radius_lr, radius_tb, 0.0f, 360.0f);
		
		for(size_t i = 1; i < props.points.size(); i++) {
			props.points[i].vertex += props.points[0].vertex;
		}
		
		props.extent.set(p.x - radius_lr, p.y - radius_tb,
						 p.x + radius_lr, p.y + radius_tb);
		
		draw_style::draw(props, args...);
	}
};
template <class draw_style>
struct gfx2d::point_compute_circle_sector {
	template<typename... Args> static void compute_and_draw(const float2& p,
															const float radius_lr,
															const float radius_tb,
															const float start_angle,
															const float end_angle,
															const Args&... args) {
		primitive_properties props(PRIMITIVE_TYPE::TRIANGLE_FAN);
		props.has_mid_point = 1;
		props.points.emplace_back(p);
		
		gfx2d::compute_ellipsoid_points(props.points, radius_lr, radius_tb, start_angle, end_angle);
		
		for(size_t i = 1; i < props.points.size(); i++) {
			props.points[i].vertex += props.points[0].vertex;
		}
		
		props.extent.set(p.x - radius_lr, p.y - radius_tb,
						 p.x + radius_lr, p.y + radius_tb);
		
		draw_style::draw(props, args...);
	}
};

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/*
 *	// draw style interface:
 *	struct draw_style_interface {
 *		static void draw(const primitive_properties& props, ...args...);
 *	};
 */

struct gfx2d::draw_style_fill {
	static void draw(const primitive_properties& props,
					 const float4& color) {
		//
		shader_helper::simple_shd->use();
		simple_shd_uniforms uniforms {
			color
		};
		shader_helper::simple_shd->set_uniforms(uniforms);
		
		// draw
		upload_points_and_draw(props);
	}
};
struct gfx2d::draw_style_gradient {
	static void draw(const primitive_properties& props,
					 const gfx2d::GRADIENT_TYPE type,
					 const float4& stops,
					 const vector<float4>& colors) {
		// draw
		const string option = (type == gfx2d::GRADIENT_TYPE::HORIZONTAL ? "gradient_horizontal" :
							   (type == gfx2d::GRADIENT_TYPE::VERTICAL ? "gradient_vertical" :
								(type == gfx2d::GRADIENT_TYPE::DIAGONAL_LR ? "gradient_diagonal_lr" : "gradient_diagonal_rl")));
		
		shader_helper::gradient_shd->use(option);
		gradient_shd_uniforms uniforms {
			{{
				colors.size() > 0 ? colors[0] : float4(0.0f),
				colors.size() > 1 ? colors[1] : float4(0.0f),
				colors.size() > 2 ? colors[2] : float4(0.0f),
				colors.size() > 3 ? colors[3] : float4(0.0f),
			}},
			stops,
			props.extent
		};
		shader_helper::gradient_shd->set_uniforms(uniforms);
		
		// draw
		upload_points_and_draw(props);
	}
};
struct gfx2d::draw_style_texture {
	// texture 2d
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, false, 0.0f, bottom_left, top_right, draw_depth, "#");
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const bool passthrough,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, false, 0.0f, bottom_left, top_right, draw_depth,
			 (passthrough ? "passthrough" : "#"));
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float4 mul_color,
					 const float4 add_color,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, false, 0.0f, mul_color, add_color, bottom_left, top_right, draw_depth, "madd_color");
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float4 mul_color,
					 const float4 add_color,
					 const gfx2d::GRADIENT_TYPE type,
					 const float4& gradient_stops,
					 const vector<float4>& gradient_colors,
					 const float4 gradient_mul_interpolator = float4(0.5f),
					 const float4 gradient_add_interpolator = float4(0.0f),
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		const string option = (type == gfx2d::GRADIENT_TYPE::HORIZONTAL ? "gradient_horizontal" :
							   (type == gfx2d::GRADIENT_TYPE::VERTICAL ? "gradient_vertical" :
								(type == gfx2d::GRADIENT_TYPE::DIAGONAL_LR ? "gradient_diagonal_lr" : "gradient_diagonal_rl")));
		draw(props, texture, false, 0.0f, mul_color, add_color, gradient_stops, gradient_colors, gradient_mul_interpolator,
			 gradient_add_interpolator, bottom_left, top_right, draw_depth, option);
	}
	
	// texture 2d array
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float layer,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, true, layer, bottom_left, top_right, draw_depth, "#");
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float layer,
					 const bool passthrough,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, true, layer, bottom_left, top_right, draw_depth,
			 (passthrough ? "passthrough" : "#"));
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float layer,
					 const float4 mul_color,
					 const float4 add_color,
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		draw(props, texture, true, layer, mul_color, add_color, bottom_left, top_right, draw_depth, "madd_color");
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const float layer,
					 const float4 mul_color,
					 const float4 add_color,
					 const gfx2d::GRADIENT_TYPE type,
					 const float4& gradient_stops,
					 const vector<float4>& gradient_colors,
					 const float4 gradient_mul_interpolator = float4(0.5f),
					 const float4 gradient_add_interpolator = float4(0.0f),
					 const coord bottom_left = coord(0.0f),
					 const coord top_right = coord(1.0f),
					 const float draw_depth = 0.0f) {
		const string option = (type == gfx2d::GRADIENT_TYPE::HORIZONTAL ? "gradient_horizontal" :
							   (type == gfx2d::GRADIENT_TYPE::VERTICAL ? "gradient_vertical" :
								(type == gfx2d::GRADIENT_TYPE::DIAGONAL_LR ? "gradient_diagonal_lr" : "gradient_diagonal_rl")));
		draw(props, texture, true, layer, mul_color, add_color, gradient_stops, gradient_colors, gradient_mul_interpolator,
			 gradient_add_interpolator, bottom_left, top_right, draw_depth, option);
	}
	
protected:
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const bool is_tex_array oclr_unused,
					 const float layer oclr_unused,
					 const coord bottom_left,
					 const coord top_right,
					 const float draw_depth,
					 const string& option) {
		// TODO: tex array handling (use tex_array option + set layer uniform)
		shader_helper::texture_shd->use(option);
		
		texture_shd_uniforms uniforms {
			props.extent,
			float4(bottom_left.u, bottom_left.v, top_right.u, top_right.v), // orientation
			draw_depth
		};
		shader_helper::texture_shd->set_uniforms(uniforms);
		oclr_pipeline->bind_image("texture", texture);
		
		// draw
		upload_points_and_draw(props);
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const bool is_tex_array oclr_unused,
					 const float layer oclr_unused,
					 const float4 mul_color,
					 const float4 add_color,
					 const coord bottom_left,
					 const coord top_right,
					 const float draw_depth,
					 const string& option) {
		// TODO: tex array handling (use tex_array option + set layer uniform)
		shader_helper::texture_shd->use(option);
		
		texture_madd_color_shd_uniforms uniforms {
			mul_color, add_color,
			props.extent,
			float4(bottom_left.u, bottom_left.v, top_right.u, top_right.v), // orientation
			draw_depth
		};
		shader_helper::texture_shd->set_uniforms(uniforms);
		oclr_pipeline->bind_image("texture", texture);
		
		// draw
		upload_points_and_draw(props);
	}
	static void draw(const primitive_properties& props,
					 const image& texture,
					 const bool is_tex_array oclr_unused,
					 const float layer oclr_unused,
					 const float4 mul_color,
					 const float4 add_color,
					 const float4& gradient_stops,
					 const vector<float4>& gradient_colors,
					 const float4 gradient_mul_interpolator,
					 const float4 gradient_add_interpolator,
					 const coord bottom_left,
					 const coord top_right,
					 const float draw_depth,
					 const string& option) {
		// TODO: tex array handling (use tex_array option + set layer uniform)
		shader_helper::texture_shd->use(option);
		
		texture_gradient_shd_uniforms uniforms {
			mul_color, add_color,
			gradient_mul_interpolator, gradient_add_interpolator,
			{{
				gradient_colors.size() > 0 ? gradient_colors[0] : float4(0.0f),
				gradient_colors.size() > 1 ? gradient_colors[1] : float4(0.0f),
				gradient_colors.size() > 2 ? gradient_colors[2] : float4(0.0f),
				gradient_colors.size() > 3 ? gradient_colors[3] : float4(0.0f),
			}},
			gradient_stops,
			props.extent,
			float4(bottom_left.u, bottom_left.v, top_right.u, top_right.v), // orientation
			draw_depth
		};
		shader_helper::texture_shd->set_uniforms(uniforms);
		oclr_pipeline->bind_image("texture", texture);
		
		// draw
		upload_points_and_draw(props);
	}
};
template <class draw_style_next>
struct gfx2d::draw_style_border {
	//! note: this requires an additional draw_style that will do the actual drawing
	template<typename... Args> static void draw(const primitive_properties& props,
												const float thickness,
												const Args&... args) {
		// duplicate and extrude/offset
		primitive_properties border_props(PRIMITIVE_TYPE::TRIANGLE_STRIP);
		border_props.extent = props.extent;
		border_props.flags = props.flags;
		border_props.has_mid_point = 0;
		const float2 center((props.extent.x + props.extent.z) * 0.5f,
							(props.extent.y + props.extent.w) * 0.5f);
		const vector<primitive_properties::primitive_point>* points = &props.points;
		const size_t orig_point_count = points->size();
		border_props.points.reserve(orig_point_count * 2 + (props.border_connect_start_end_point ? 2 : 0)); // we'll need twice as much
		
		vector<primitive_properties::primitive_point> swapped_points;
		if(props.border_swap_strip_points) {
			swapped_points.assign(cbegin(*points), cend(*points));
			std::swap(swapped_points[2], swapped_points[3]);
			points = &swapped_points;
		}
		
		for(size_t i = props.has_mid_point; // ignore mid point(s) if it has one
			i < orig_point_count; i++) {
			// to do this correctly, we need the previous and next point ...
			const float2& cur_point = (*points)[i].vertex;
			
			// (mp/#0) -> (sp/#1) -> #2 -> #3 -> ... -> #n-2 -> (ep/#n-1)
			const float2& prev_point = (*points)[i > props.has_mid_point ? i-1 : orig_point_count - (1 + props.has_mid_point + props.has_equal_start_end_point)].vertex;
			const float2& next_point = (*points)[i < (orig_point_count-1) ? i+1 : (props.has_mid_point * 2) + props.has_equal_start_end_point].vertex;
			
			// ... and compute the normal of those three points (from the two vectors outgoing from the current point)
			const float2 v0(float2(prev_point.x - cur_point.x,
								   prev_point.y - cur_point.y).normalize());
			const float2 v1(float2(next_point.x - cur_point.x,
								   next_point.y - cur_point.y).normalize());
			const float2 normal((v0 + v1).normalize());
			
			//
			border_props.points.emplace_back(cur_point);
			border_props.points.emplace_back(float2(cur_point.x, cur_point.y) +
											 normal * thickness);
		}
		
		if(props.border_connect_start_end_point) {
			border_props.points.emplace_back(border_props.points[0]);
			border_props.points.emplace_back(border_props.points[1]);
		}
		
		// no point in rendering no points
		if(border_props.points.empty()) return;
		
		// draw (note: this will always render a triangle strip)
		draw_style_next::draw(border_props, args...);
	}
};

#endif
