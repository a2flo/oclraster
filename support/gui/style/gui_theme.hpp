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

#ifndef __OCLRASTER_SUPPORT_GUI_THEME_HPP__
#define __OCLRASTER_SUPPORT_GUI_THEME_HPP__

#include "oclraster_support/global.hpp"
#include "core/xml.hpp"
#include "gui_color_scheme.hpp"
#include "rendering/gfx2d.hpp"

class xml;
class font_manager;
class font;
class gui_theme {
public:
	gui_theme(font_manager* fm);
	~gui_theme();
	
	bool load(const string& filename);
	void reload();
	
	// ui object type, state of that object, screen x/y offset, draw size/extent,
	// text lookup: takes an identifier and returns the associated string
	void draw(const string& type, const string& state,
			  const float2& offset, const float2& size,
			  const bool clear = true, const bool scissor = true,
			  std::function<string(const string&)> text_lookup = [](const string& str floor_unused){return "";},
			  std::function<image*(const string&)> texture_lookup = [](const string& tex_name floor_unused){return nullptr;});
	
	const gui_color_scheme& get_color_scheme() const;
	
	static float point_to_pixel(const float& pt);
	
	//
	enum class PRIMITIVE_TYPE : unsigned int {
		POINT,
		LINE,
		TRIANGLE,
		RECTANGLE,
		ROUNDED_RECTANGLE,
		CIRCLE,
		CIRCLE_SECTOR,
		ELLIPSOID,
		ELLIPSOID_SECTOR,
		TEXT,
		//POLYGON, // TODO: allow/implement this?
	};
	
	enum class DRAW_STYLE : unsigned int {
		FILL,
		GRADIENT,
		TEXTURE,
		BORDER_FILL,
		BORDER_GRADIENT,
		BORDER_TEXTURE,
		TEXT,
	};
	
	//
	enum class VALUE_TYPE {
		PIXEL,
		PERCENTAGE,
		POINT,
		CENTER,
	};
	struct ui_float {
		float ui_value;
		float value;
		VALUE_TYPE type;
		void compute(const float& size);
		constexpr ui_float(const float& ui_value_, const VALUE_TYPE& type_) noexcept :
		ui_value(ui_value_), value(0.0f), type(type_) {}
		constexpr ui_float(ui_float&& f) noexcept : ui_value(f.ui_value), value(f.value), type(f.type) {}
		constexpr ui_float(const ui_float& f) noexcept : ui_value(f.ui_value), value(f.value), type(f.type) {}
	};
	struct ui_float2 {
		array<ui_float, 2> ui_value;
		float2 value;
		
		void compute(const float2& size) {
			ui_value[0].compute(size.x);
			ui_value[1].compute(size.y);
			value.x = ui_value[0].value;
			value.y = ui_value[1].value;
		}
		
		constexpr ui_float2(const float& ui_value_x, const VALUE_TYPE& type_x,
								const float& ui_value_y, const VALUE_TYPE& type_y) noexcept :
		ui_value { { ui_float(ui_value_x, type_x), ui_float(ui_value_y, type_y) } }, value(0.0f, 0.0f) {}
		constexpr ui_float2(ui_float2&& f) noexcept :
		ui_value { { f.ui_value[0], f.ui_value[1] } }, value(f.value) {}
		constexpr ui_float2(const ui_float2& f) noexcept :
		ui_value { { f.ui_value[0], f.ui_value[1] } }, value(f.value) {}
	};
	enum class COLOR_TYPE {
		SCHEME,
		RAW,
	};
	struct ui_color {
		float4 value;
		COLOR_TYPE type;
		string ref;
		
		void compute(const gui_color_scheme& color_scheme) {
			if(type == COLOR_TYPE::SCHEME) {
				value = color_scheme.get(ref);
			}
		}
		
		ui_color(const float4& color) noexcept :
		value { color }, type { COLOR_TYPE::RAW }, ref { "" } {}
		ui_color(const string& color) noexcept :
		value { 0.0f }, type { COLOR_TYPE::SCHEME }, ref { color } {}
		ui_color(ui_color&& color) noexcept :
		value { color.value }, type { color.type }, ref { color.ref } {}
		ui_color(const ui_color& color) noexcept :
		value { color.value }, type { color.type }, ref { color.ref } {}
	};
	
	//
	struct point_compute_data {
		// empty
	};
	struct pc_point : public point_compute_data {
		ui_float2 point;
		pc_point(ui_float2&& p) : point(p) {}
	};
	struct pc_line : public point_compute_data {
		ui_float2 start_point, end_point;
		ui_float thickness;
		pc_line(ui_float2&& sp, ui_float2&& ep, ui_float&& thck) : start_point(sp), end_point(ep), thickness(thck) {}
	};
	struct pc_triangle : public point_compute_data {
		ui_float2 point_0, point_1, point_2;
		pc_triangle(ui_float2&& p0, ui_float2&& p1, ui_float2&& p2) : point_0(p0), point_1(p1), point_2(p2) {}
	};
	struct pc_rectangle : public point_compute_data {
		ui_float2 start_point, end_point;
		pc_rectangle(ui_float2&& sp, ui_float2&& ep) : start_point(sp), end_point(ep) {}
	};
	struct pc_rounded_rectangle : public point_compute_data {
		ui_float2 start_point, end_point;
		ui_float radius;
		gfx2d::CORNER corners;
		pc_rounded_rectangle(ui_float2&& sp, ui_float2&& ep, ui_float&& rd, gfx2d::CORNER&& crnrs) : start_point(sp), end_point(ep), radius(rd), corners(crnrs) {}
	};
	struct pc_circle : public point_compute_data {
		ui_float2 point;
		ui_float radius;
		pc_circle(ui_float2&& p, ui_float&& rd) : point(p), radius(rd) {}
	};
	struct pc_circle_sector : public point_compute_data {
		ui_float2 point;
		ui_float radius;
		ui_float start_angle, end_angle;
		pc_circle_sector(ui_float2&& p, ui_float&& rd, ui_float&& sangle, ui_float&& eangle) : point(p), radius(rd), start_angle(sangle), end_angle(eangle) {}
	};
	struct pc_ellipsoid : public point_compute_data {
		ui_float2 point;
		ui_float radius_lr, radius_tb;
		pc_ellipsoid(ui_float2&& p, ui_float2&& r_lr_tb) : point(p),
		radius_lr(ui_float(r_lr_tb.ui_value[0].ui_value, r_lr_tb.ui_value[0].type)),
		radius_tb(ui_float(r_lr_tb.ui_value[1].ui_value, r_lr_tb.ui_value[1].type)) {}
	};
	struct pc_ellipsoid_sector : public point_compute_data {
		ui_float2 point;
		ui_float radius_lr, radius_tb;
		ui_float start_angle, end_angle;
		pc_ellipsoid_sector(ui_float2&& p, ui_float2&& r_lr_tb, ui_float&& sangle, ui_float&& eangle) : point(p),
		radius_lr(ui_float(r_lr_tb.ui_value[0].ui_value, r_lr_tb.ui_value[0].type)),
		radius_tb(ui_float(r_lr_tb.ui_value[1].ui_value, r_lr_tb.ui_value[1].type)),
		start_angle(sangle), end_angle(eangle) {}
	};
	struct pc_text : public point_compute_data {
		ui_float2 position;
		const string identifier;
		pc_text(ui_float2&& p, string&& identifier_) : position(p), identifier(identifier_) {}
	};
	
	//
	struct draw_style_data {
		// empty
	};
	struct ds_fill : public draw_style_data {
		ui_color color;
		ds_fill(ui_color&& col) : color(col) {}
	};
	struct ds_gradient : public draw_style_data {
		gfx2d::GRADIENT_TYPE type;
		float4 stops;
		vector<ui_color> colors;
		
		ds_gradient() : type(gfx2d::GRADIENT_TYPE::HORIZONTAL), stops(float4(0.0f)), colors(vector<ui_color> {}) {}
		ds_gradient(gfx2d::GRADIENT_TYPE&& type_, float4&& stops_, vector<ui_color>&& colors_) :
		type(type_), stops(stops_), colors(colors_) {}
	};
	struct ds_texture : public draw_style_data {
		image* texture; // nullptr: external/dynamic texture
		const string texture_name;
		float2 bottom_left;
		float2 top_right;
		float depth;
		bool passthrough;
		ui_color mul_color;
		ui_color add_color;
		
		const bool is_gradient;
		float4 gradient_mul_interpolator;
		float4 gradient_add_interpolator;
		ds_gradient gradient;
		
		ds_texture(image*&& texture_, string&& texture_name_, float2&& bottom_left_, float2&& top_right_, float&& depth_, bool&& passthrough_, ui_color&& mul_color_, ui_color&& add_color_, bool&& is_gradient_, float4&& gradient_mul_interpolator_, float4&& gradient_add_interpolator_, ds_gradient&& gradient_) :
		texture(texture_), texture_name(texture_name_), bottom_left(bottom_left_), top_right(top_right_), depth(depth_), passthrough(passthrough_), mul_color(mul_color_), add_color(add_color_), is_gradient(is_gradient_), gradient_mul_interpolator(gradient_mul_interpolator_), gradient_add_interpolator(gradient_add_interpolator_), gradient(gradient_)
		{}
	};
	struct ds_border_fill : public ds_fill {
		ui_float thickness;
		ds_border_fill(ui_float&& thickness_, ui_color&& col) :
		ds_fill { std::move(col) }, thickness(thickness_) {}
	};
	struct ds_border_gradient : public ds_gradient {
		ui_float thickness;
		ds_border_gradient(ui_float&& thickness_, gfx2d::GRADIENT_TYPE&& type_, float4&& stops_, vector<ui_color>&& colors_) :
		ds_gradient { std::move(type_), std::move(stops_), std::move(colors_) }, thickness(thickness_) {}
	};
	struct ds_border_texture : public ds_texture {
		ui_float thickness;
		ds_border_texture(ui_float&& thickness_, image*&& texture_, string&& texture_name_, float2&& bottom_left_, float2&& top_right_, float&& depth_, bool&& passthrough_, ui_color&& mul_color_, ui_color&& add_color_, bool&& is_gradient_, float4&& gradient_mul_interpolator_, float4&& gradient_add_interpolator_, ds_gradient&& gradient_) :
		ds_texture { std::move(texture_), std::move(texture_name_), std::move(bottom_left_), std::move(top_right_), std::move(depth_), std::move(passthrough_), std::move(mul_color_), std::move(add_color_), std::move(is_gradient_), std::move(gradient_mul_interpolator_), std::move(gradient_add_interpolator_), std::move(gradient_) },
		thickness(thickness_) {}
	};
	struct ds_text : public draw_style_data {
		ui_color color;
		ds_text(ui_color&& col) : color(col) {}
	};
	
	struct primitive_data {
		PRIMITIVE_TYPE type;
		DRAW_STYLE style;
		unique_ptr<point_compute_data> pdata;
		unique_ptr<draw_style_data> ddata;
	};
	
protected:
	font_manager* fm;
	xml* x;
	font* fnt;
	gui_color_scheme scheme;
	string filename = "";
	
	//
	typedef std::function<void(const pnt& offset, const size2& size)> gui_ui_object_draw_function;
	struct gui_ui_object {
		const string type;
		struct state {
			vector<primitive_data> primitives;
		};
		unordered_map<string, unique_ptr<state>> states;
		gui_ui_object(const string& type_) : type(type_) {}
	};
	unordered_map<string, unique_ptr<gui_ui_object>> ui_objects;
	
	//
	bool load_ui_object(const string& type, const string& obj_filename);
	
	void process_state(gui_ui_object::state* st, const xml::xml_node* node);
	void process_primitive(gui_ui_object::state* st, const xml::xml_node* node);
	
	unique_ptr<point_compute_data> process_point_compute_data(const PRIMITIVE_TYPE primitive, const xml::xml_node* node);
	unique_ptr<draw_style_data> process_draw_style_data(const DRAW_STYLE style, const xml::xml_node* node);
	
	// text cache
	// TODO: manage the cache (-> delete data/ubo after text isn't used for a certain time)
	unordered_map<string, pair<pair<opencl::buffer_object*, unsigned int>, float2>> text_cache;
	
	// TODO: texture "cache"/container (texman acts as a cache already)
	
};

#endif
