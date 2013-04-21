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

#include "gui_theme.h"
#include "font_manager.h"
#include "font.h"
#include "oclraster_support.h"
#include "rendering/texman.h"

#define A2E_THEME_VERSION 1
#define A2E_UI_OBJECT_VERSION 1

// some helper functions:
static float screen_dpi = 72.0f;
float gui_theme::point_to_pixel(const float& pt) {
	return pt * (screen_dpi / 72.0f);
}

// converts a single "number(%|pt)" to an ui_float
static pair<float, gui_theme::VALUE_TYPE> str_to_ui_float_pair(const string& val_str) {
	const auto at_pos(val_str.find("@"));
	if(at_pos != string::npos) {
		return { 0.0f, gui_theme::VALUE_TYPE::CENTER };
	}
	
	const auto perc_pos(val_str.find("%"));
	if(perc_pos != string::npos) {
		// [0, 100] -> [0, 1]
		const float scaled_value(string2float(val_str.substr(0, perc_pos)) * 0.01f);
		return { scaled_value, gui_theme::VALUE_TYPE::PERCENTAGE };
	}
	
	const auto pt_pos(val_str.find("pt"));
	if(pt_pos != string::npos) {
		return { string2float(val_str), gui_theme::VALUE_TYPE::POINT };
	}
	
	return { string2float(val_str), gui_theme::VALUE_TYPE::PIXEL };
};
static gui_theme::ui_float str_to_ui_float(const pair<float, gui_theme::VALUE_TYPE>& uif_pair) {
	return gui_theme::ui_float { uif_pair.first, uif_pair.second };
};

// converts "number(%|pt),number(%|pt)" to an ui_float2
static gui_theme::ui_float2 str_to_ui_float2(const string& str) {
	const auto comma_pos(str.find(","));
	
	if(comma_pos == string::npos) {
		// both components have the same value
		const auto value(str_to_ui_float_pair(str));
		return gui_theme::ui_float2 { value.first, value.second, value.first, value.second };
	}
	
	// split at ','
	const auto value_x(str_to_ui_float_pair(str.substr(0, comma_pos)));
	const auto value_y(str_to_ui_float_pair(str.substr(comma_pos + 1, str.size() - comma_pos - 1)));
	return gui_theme::ui_float2 { value_x.first, value_x.second, value_y.first, value_y.second };
};

//
gui_theme::gui_theme(font_manager* fm_) : fm(fm_), x(oclraster::get_xml()), scheme() {
	fnt = fm->get_font("SYSTEM_SANS_SERIF");
	screen_dpi = oclraster::get_dpi();
}

gui_theme::~gui_theme() {
}

const gui_color_scheme& gui_theme::get_color_scheme() const {
	return scheme;
}

void gui_theme::reload() {
	if(filename == "") {
		oclr_error("no theme has been loaded yet!");
		return;
	}
	
	// clear old data
	ui_objects.clear();
	load(filename);
}

bool gui_theme::load(const string& filename_) {
	xml::xml_doc ui_doc = x->process_file(oclraster::data_path(filename_), false); // TODO: DTD!
	if(!ui_doc.valid || filename_.rfind("/") == string::npos) {
		oclr_error("couldn't process theme file %s!", filename_);
		return false;
	}
	const string theme_path = filename_.substr(0, filename_.rfind("/") + 1);
	
	const size_t doc_version = ui_doc.get<size_t>("a2e_theme.version");
	if(doc_version != A2E_THEME_VERSION) {
		oclr_error("invalid theme version: %u (should be %u)!",
				  doc_version, A2E_THEME_VERSION);
		return false;
	}
	
	// load color scheme
	const string scheme_filename = ui_doc.get<string>("a2e_theme.colors");
	if(scheme_filename == "INVALID") {
		oclr_error("no color scheme specified in theme \"%s\"!", filename_);
		return false;
	}
	if(!scheme.load(theme_path + scheme_filename)) {
		oclr_error("failed to load color scheme \"%s\" for theme \"%s\"!", scheme_filename, filename_);
		return false;
	}
	
	//
	const xml::xml_node* theme_node = ui_doc.get_node("a2e_theme");
	if(theme_node == nullptr || theme_node->children.empty()) {
		oclr_error("theme \"%s\" is empty!", filename_);
		return false;
	}
	
	// process nodes / ui objects
	for(const auto& node : theme_node->children) {
		if(node.first == "object") {
			const string& obj_filename = (*node.second)["file"];
			const string& obj_type = (*node.second)["type"];
			if(obj_filename != "INVALID" && obj_type != "INVALID") {
				load_ui_object(obj_type, theme_path + obj_filename);
			}
			else {
				if(obj_filename == "INVALID") {
					oclr_error("filename is missing for object in theme \"%s\"!", filename_);
				}
				else {
					oclr_error("type is missing for object in theme \"%s\"!", filename_);
				}
			}
		}
		else if(node.first[0] != '#') {
			oclr_error("unknown node \"%s\" in theme \"%s\"!", node.first, filename_);
		}
	}
	filename = filename_;
	return true;
}

bool gui_theme::load_ui_object(const string& type, const string& obj_filename) {
	xml::xml_doc ui_object_doc = x->process_file(oclraster::data_path(obj_filename), false); // TODO: DTD!
	if(!ui_object_doc.valid) {
		oclr_error("couldn't process ui object file %s!", obj_filename);
		return false;
	}
	
	const size_t obj_version = ui_object_doc.get<size_t>("a2e_ui_object.version");
	if(obj_version != A2E_UI_OBJECT_VERSION) {
		oclr_error("invalid ui object version: %u (should be %u)!",
				  obj_version, A2E_UI_OBJECT_VERSION);
		return false;
	}
	
	//
	const xml::xml_node* obj_node = ui_object_doc.get_node("a2e_ui_object");
	if(obj_node == nullptr || obj_node->children.empty()) {
		oclr_error("ui object \"%s\" is empty!", obj_filename);
		return false;
	}
	
	// process nodes
	gui_ui_object* obj { new gui_ui_object(type) };
	for(const auto& node : obj_node->children) {
		if(node.first == "state" && (*node.second)["type"] != "INVALID") {
			gui_ui_object::state* st = new gui_ui_object::state();
			process_state(st, node.second);
			obj->states.insert(make_pair((*node.second)["type"], unique_ptr<gui_ui_object::state>(st)));
		}
		else if(node.first[0] != '#') {
			oclr_error("unknown node \"%s\" in ui object \"%s\"!", node.first, obj_filename);
		}
	}
	ui_objects.insert(make_pair(type, unique_ptr<gui_ui_object>(obj)));
	return true;
}

void gui_theme::process_state(gui_ui_object::state* st, const xml::xml_node* node) {
	// process child nodes
	for(const auto& child : node->children) {
		if(child.first[0] != '#') {
			process_primitive(st, child.second);
		}
	}
}

void gui_theme::process_primitive(gui_ui_object::state* st, const xml::xml_node* node) {
	// get/check primitive type
	static const unordered_map<string, PRIMITIVE_TYPE> primitive_lookup {
		{ "point", PRIMITIVE_TYPE::POINT },
		{ "line", PRIMITIVE_TYPE::LINE },
		{ "triangle", PRIMITIVE_TYPE::TRIANGLE },
		{ "rectangle", PRIMITIVE_TYPE::RECTANGLE },
		{ "rounded_rectangle", PRIMITIVE_TYPE::ROUNDED_RECTANGLE },
		{ "circle", PRIMITIVE_TYPE::CIRCLE },
		{ "circle_sector", PRIMITIVE_TYPE::CIRCLE_SECTOR },
		{ "ellipsoid", PRIMITIVE_TYPE::ELLIPSOID },
		{ "ellipsoid_sector", PRIMITIVE_TYPE::ELLIPSOID_SECTOR },
		{ "text", PRIMITIVE_TYPE::TEXT },
	};
	const auto primitive_iter = primitive_lookup.find(node->name());
	if(primitive_iter == primitive_lookup.cend()) {
		oclr_error("invalid primitive type specified: %s!", node->name());
		return;
	}
	const PRIMITIVE_TYPE primitive { primitive_iter->second };
	
	// get/check primitive style
	static const unordered_map<string, DRAW_STYLE> style_lookup {
		{ "fill", DRAW_STYLE::FILL },
		{ "gradient", DRAW_STYLE::GRADIENT },
		{ "texture", DRAW_STYLE::TEXTURE },
		{ "border_fill", DRAW_STYLE::BORDER_FILL },
		{ "border_gradient", DRAW_STYLE::BORDER_GRADIENT },
		{ "border_texture", DRAW_STYLE::BORDER_TEXTURE },
		{ "text", DRAW_STYLE::TEXT },
	};
	const string style_str(primitive != PRIMITIVE_TYPE::TEXT ? (*node)["style"] : "text");
	if(style_str == "INVALID") {
		oclr_error("no style specified for primitive: %s", node->name());
		return;
	}
	const auto style_iter = style_lookup.find(style_str);
	if(style_iter == style_lookup.cend()) {
		oclr_error("invalid style (%s) specified for primitive: %s", style_str, node->name());
		return;
	}
	const DRAW_STYLE style { style_iter->second };
	
	// make sure primitive and style tags got the necessary attributes (not completely achievable with dtd)
	if((style == DRAW_STYLE::TEXTURE || style == DRAW_STYLE::BORDER_TEXTURE) &&
	   (*node)["name"] == "INVALID") {
		oclr_error("no texture filename specified for primitive: %s!", node->name());
		return;
	}
	
	// and load/process:
	st->primitives.emplace_back(primitive_data {
		primitive, style,
		process_point_compute_data(primitive, node),
		process_draw_style_data(style, node)
	});
}

unique_ptr<gui_theme::point_compute_data> gui_theme::process_point_compute_data(const PRIMITIVE_TYPE primitive, const xml::xml_node* node) {
	switch(primitive) {
		case PRIMITIVE_TYPE::POINT:
			return make_unique<pc_point>(str_to_ui_float2((*node)["position"]));
		case PRIMITIVE_TYPE::LINE:
			return make_unique<pc_line>(str_to_ui_float2((*node)["start"]),
										str_to_ui_float2((*node)["end"]),
										str_to_ui_float(str_to_ui_float_pair((*node)["thickness"])));
		case PRIMITIVE_TYPE::TRIANGLE:
			return make_unique<pc_triangle>(str_to_ui_float2((*node)["p0"]),
											str_to_ui_float2((*node)["p1"]),
											str_to_ui_float2((*node)["p2"]));
		case PRIMITIVE_TYPE::RECTANGLE:
			return make_unique<pc_rectangle>(str_to_ui_float2((*node)["start"]),
											 str_to_ui_float2((*node)["end"]));
		case PRIMITIVE_TYPE::ROUNDED_RECTANGLE: {
			// converts the corners list string to a corner enum
			const auto parse_corners = [](const string& corners_str) -> gfx2d::CORNER {
				if(corners_str == "INVALID") return gfx2d::CORNER::ALL;
				const vector<string> corner_tokens(core::tokenize(corners_str, ','));
				gfx2d::CORNER corners = gfx2d::CORNER::NONE;
				for(const auto& token : corner_tokens) {
					if(token == "all") {
						corners |= gfx2d::CORNER::ALL;
						break;
					}
					else if(token == "top_right") corners |= gfx2d::CORNER::TOP_RIGHT;
					else if(token == "bottom_right") corners |= gfx2d::CORNER::BOTTOM_RIGHT;
					else if(token == "bottom_left") corners |= gfx2d::CORNER::BOTTOM_LEFT;
					else if(token == "top_left") corners |= gfx2d::CORNER::TOP_LEFT;
					else {
						oclr_error("unknown corner token: %s!", token);
					}
				}
				return corners;
			};
			
			return make_unique<pc_rounded_rectangle>(str_to_ui_float2((*node)["start"]),
													 str_to_ui_float2((*node)["end"]),
													 str_to_ui_float(str_to_ui_float_pair((*node)["radius"])),
													 parse_corners((*node)["corners"]));
		}
		case PRIMITIVE_TYPE::CIRCLE:
			return make_unique<pc_circle>(str_to_ui_float2((*node)["point"]),
										  str_to_ui_float(str_to_ui_float_pair((*node)["radius"])));
		case PRIMITIVE_TYPE::CIRCLE_SECTOR:
			return make_unique<pc_circle_sector>(str_to_ui_float2((*node)["point"]),
												 str_to_ui_float(str_to_ui_float_pair((*node)["radius"])),
												 str_to_ui_float(str_to_ui_float_pair((*node)["start"])),
												 str_to_ui_float(str_to_ui_float_pair((*node)["stop"])));
		case PRIMITIVE_TYPE::ELLIPSOID:
			return make_unique<pc_ellipsoid>(str_to_ui_float2((*node)["point"]),
											 str_to_ui_float2((*node)["radius"]));
		case PRIMITIVE_TYPE::ELLIPSOID_SECTOR:
			return make_unique<pc_ellipsoid_sector>(str_to_ui_float2((*node)["point"]),
													str_to_ui_float2((*node)["radius"]),
													str_to_ui_float(str_to_ui_float_pair((*node)["start"])),
													str_to_ui_float(str_to_ui_float_pair((*node)["stop"])));
		case PRIMITIVE_TYPE::TEXT:
			return make_unique<pc_text>(str_to_ui_float2((*node)["position"]),
										string((*node)["id"]));
	}
	oclr_unreachable();
}

unique_ptr<gui_theme::draw_style_data> gui_theme::process_draw_style_data(const gui_theme::DRAW_STYLE style, const xml::xml_node* node) {
	//
	const auto str_to_float4 = [](const string& float4_str) -> float4 {
		const vector<string> float4_tokens { core::tokenize(float4_str, ',') };
		if(float4_tokens.size() != 4) {
			oclr_error("invalid float4 token count: %u!", float4_tokens.size());
			return float4(0.0f, 1.0f, 0.0f, 1.0f); // green -> invalid color/float4
		}
		return float4(string2float(float4_tokens[0]),
					  string2float(float4_tokens[1]),
					  string2float(float4_tokens[2]),
					  string2float(float4_tokens[3]));
	};
	const auto str_to_ui_color = [&str_to_float4](const string& color_str) -> ui_color {
		const auto comma_pos(color_str.find(","));
		
		// no comma -> must be a scheme reference
		if(comma_pos == string::npos) {
			return ui_color { color_str }; // validity must be checked later
		}
		// comma -> must be a raw color
		return ui_color { str_to_float4(color_str) };
	};
	
	// gradient helpers:
	const auto str_to_gradient = [](const string& gradient_str) -> gfx2d::GRADIENT_TYPE {
		static const unordered_map<string, gfx2d::GRADIENT_TYPE> types {
			{ "horizontal", gfx2d::GRADIENT_TYPE::HORIZONTAL },
			{ "vertical", gfx2d::GRADIENT_TYPE::VERTICAL },
			{ "diagonal_lr", gfx2d::GRADIENT_TYPE::DIAGONAL_LR },
			{ "diagonal_rl", gfx2d::GRADIENT_TYPE::DIAGONAL_RL },
		};
		
		const auto iter = types.find(gradient_str);
		if(iter == types.cend()) {
			oclr_error("invalid gradient type: %s!", gradient_str);
			return gfx2d::GRADIENT_TYPE::HORIZONTAL;
		}
		return iter->second;
	};
	
	const auto str_to_gradient_stops = [](const string& stops_str) -> float4 {
		const auto tokens = core::tokenize(stops_str, ',');
		float4 ret(0.0f);
		for(size_t i = 0; i < std::min(tokens.size(), (size_t)4); i++) {
			ret[i] = string2float(tokens[i]);
		}
		return ret;
	};
	
	const auto str_to_gradient_colors = [&str_to_ui_color](const string& gradient_colors) -> vector<ui_color> {
		const auto color_tokens = core::tokenize(gradient_colors, ';');
		vector<ui_color> colors;
		for(size_t i = 0; i < std::min(color_tokens.size(), (size_t)4); i++) {
			colors.emplace_back(str_to_ui_color(color_tokens[i]));
		}
		return colors;
	};
	
	// texture helpers:
	const auto str_to_coords = [](const string& coords_str) -> pair<float2, float2> {
		if(coords_str == "INVALID") return make_pair(float2(0.0f), float2(1.0f)); // default
		
		const auto coords_tokens = core::tokenize(coords_str, ';');
		if(coords_tokens.size() != 2) {
			oclr_error("invalid coord token count (%u) for coords: %s!", coords_tokens.size(), coords_str);
			return make_pair(float2(0.0f), float2(1.0f));
		}
		
		const auto bottom_left_tokens = core::tokenize(coords_tokens[0], ',');
		const auto top_right_tokens = core::tokenize(coords_tokens[1], ',');
		return make_pair(bottom_left_tokens.size() >= 2 ?
						 float2(string2float(bottom_left_tokens[0]), string2float(bottom_left_tokens[1])) :
						 float2(string2float(bottom_left_tokens[0])),
						 top_right_tokens.size() >= 2 ?
						 float2(string2float(top_right_tokens[0]), string2float(top_right_tokens[1])) :
						 float2(string2float(top_right_tokens[0])));
	};
	
	//
	switch(style) {
		case DRAW_STYLE::FILL:
			return make_unique<ds_fill>(str_to_ui_color((*node)["color"]));
		case DRAW_STYLE::BORDER_FILL:
			return make_unique<ds_border_fill>(str_to_ui_float(str_to_ui_float_pair((*node)["thickness"])),
											   str_to_ui_color((*node)["color"]));
		case DRAW_STYLE::GRADIENT:
			return make_unique<ds_gradient>(str_to_gradient((*node)["gradient"]),
											str_to_gradient_stops((*node)["stops"]),
											str_to_gradient_colors((*node)["colors"]));
		case DRAW_STYLE::BORDER_GRADIENT:
			return make_unique<ds_border_gradient>(str_to_ui_float(str_to_ui_float_pair((*node)["thickness"])),
												   str_to_gradient((*node)["gradient"]),
												   str_to_gradient_stops((*node)["stops"]),
												   str_to_gradient_colors((*node)["colors"]));
		case DRAW_STYLE::BORDER_TEXTURE:
		case DRAW_STYLE::TEXTURE: {
			string tex_name = (*node)["name"];
			image* texture = nullptr;
			if(tex_name.find(".png") != string::npos) {
				// TODO: stores textures; add/allow internal engine image names?
				texture = texture_manager::add_texture(oclraster::data_path(tex_name));
				tex_name = "";
			}
			auto coords = str_to_coords((*node)["coords"]);
			float depth = ((*node)["depth"] == "INVALID" ? 0.0f : string2float((*node)["depth"]));
			bool passthrough = ((*node)["passthrough"] == "INVALID" ? false : string2bool((*node)["passthrough"]));
			bool is_gradient = ((*node)["gradient"] != "INVALID");
			const bool is_mul_color = ((*node)["mul"] != "INVALID");
			const bool is_add_color = ((*node)["add"] != "INVALID");
			const bool is_grad_mul_interp = ((*node)["mul_interpolator"] != "INVALID");
			const bool is_grad_add_interp = ((*node)["add_interpolator"] != "INVALID");
			
			if(style == DRAW_STYLE::TEXTURE) {
				return make_unique<ds_texture>(std::move(texture),
											   std::move(tex_name),
											   std::move(coords.first),
											   std::move(coords.second),
											   std::move(depth),
											   std::move(passthrough),
											   is_mul_color ?
											   str_to_ui_color((*node)["mul"]) : ui_color(float4(1.0f)),
											   is_add_color ?
											   str_to_ui_color((*node)["add"]) : ui_color(float4(0.0f)),
											   std::move(is_gradient),
											   is_grad_mul_interp ?
											   str_to_float4((*node)["mul_interpolator"]) : float4(0.5f),
											   is_grad_add_interp ?
											   str_to_float4((*node)["add_interpolator"]) : float4(0.0f),
											   is_gradient ?
											   ds_gradient(str_to_gradient((*node)["gradient"]),
														   str_to_gradient_stops((*node)["stops"]),
														   str_to_gradient_colors((*node)["colors"])) :
											   ds_gradient());
			}
			return make_unique<ds_border_texture>(str_to_ui_float(str_to_ui_float_pair((*node)["thickness"])),
												  std::move(texture),
												  std::move(tex_name),
												  std::move(coords.first),
												  std::move(coords.second),
												  std::move(depth),
												  std::move(passthrough),
												  is_mul_color ?
												  str_to_ui_color((*node)["mul"]) : ui_color(float4(1.0f)),
												  is_add_color ?
												  str_to_ui_color((*node)["add"]) : ui_color(float4(0.0f)),
												  std::move(is_gradient),
												  is_grad_mul_interp ?
												  str_to_float4((*node)["mul_interpolator"]) : float4(0.5f),
												  is_grad_add_interp ?
												  str_to_float4((*node)["add_interpolator"]) : float4(0.0f),
												  is_gradient ?
												  ds_gradient(str_to_gradient((*node)["gradient"]),
															  str_to_gradient_stops((*node)["stops"]),
															  str_to_gradient_colors((*node)["colors"])) :
												  ds_gradient());
		}
		case DRAW_STYLE::TEXT:
			return make_unique<ds_text>(str_to_ui_color((*node)["color"]));
	}
	oclr_unreachable();
}

//
void gui_theme::draw(const string& type, const string& state,
					 const float2& offset, const float2& size,
					 const bool clear, const bool scissor,
					 std::function<string(const string&)> text_lookup,
					 std::function<image*(const string&)> texture_lookup) {
	const auto iter = ui_objects.find(type);
	if(iter == ui_objects.cend()) {
		oclr_error("invalid type: %s!", type);
		return;
	}
	
	//
	gui_ui_object* ui_obj = iter->second.get();
	const auto state_iter = ui_obj->states.find(state);
	if(state_iter == ui_obj->states.cend()) {
		oclr_error("invalid state: %s!", state);
		return;
	}
	
	// set scissor test rect
	if(scissor) {
		glScissor(floorf(offset.x), floorf(offset.y),
				  ceilf(size.x), ceilf(size.y));
	}
	if(clear) {
		oclraster_support::get_pipeline()->get_bound_framebuffer()->clear();
	}
	
	//
	gui_ui_object::state* st = state_iter->second.get();
	const float size_avg = (size.x + size.y) * 0.5f;
	for(const auto& prim : st->primitives) {
		// helpers:
		static gfx2d::primitive_properties pprops;
		struct compute_only_draw_style {
			static void draw(gfx2d::primitive_properties& props) {
				pprops = std::move(props);
			}
		};
		
		// compute sizes and create primitive_properties for drawing
		switch(prim.type) {
			case PRIMITIVE_TYPE::POINT: {
				auto pd = (pc_point*)&*prim.pdata;
				pd->point.compute(size);
				gfx2d::point_compute_point<compute_only_draw_style>::compute_and_draw(pd->point.value + offset);
			}
			break;
			case PRIMITIVE_TYPE::LINE: {
				auto pd = (pc_line*)&*prim.pdata;
				pd->start_point.compute(size);
				pd->end_point.compute(size);
				pd->thickness.compute(size_avg);
				gfx2d::point_compute_line<compute_only_draw_style>::compute_and_draw(pd->start_point.value + offset,
																					 pd->end_point.value + offset,
																					 pd->thickness.value);
			}
			break;
			case PRIMITIVE_TYPE::TRIANGLE: {
				auto pd = (pc_triangle*)&*prim.pdata;
				pd->point_0.compute(size);
				pd->point_1.compute(size);
				pd->point_2.compute(size);
				gfx2d::point_compute_triangle<compute_only_draw_style>::compute_and_draw(pd->point_0.value + offset,
																						 pd->point_1.value + offset,
																						 pd->point_2.value + offset);
			}
			break;
			case PRIMITIVE_TYPE::RECTANGLE: {
				auto pd = (pc_rectangle*)&*prim.pdata;
				pd->start_point.compute(size);
				pd->end_point.compute(size);
				gfx2d::point_compute_rectangle<compute_only_draw_style>::compute_and_draw(rect(pd->start_point.value.x + offset.x,
																							   pd->start_point.value.y + offset.y,
																							   pd->end_point.value.x + offset.x,
																							   pd->end_point.value.y + offset.y));
			}
			break;
			case PRIMITIVE_TYPE::ROUNDED_RECTANGLE: {
				auto pd = (pc_rounded_rectangle*)&*prim.pdata;
				pd->start_point.compute(size);
				pd->end_point.compute(size);
				pd->radius.compute(size_avg);
				const rect rec(pd->start_point.value.x + offset.x,
							   pd->start_point.value.y + offset.y,
							   pd->end_point.value.x + offset.x,
							   pd->end_point.value.y + offset.y);
				gfx2d::point_compute_rounded_rectangle<compute_only_draw_style>::compute_and_draw(rec, pd->radius.value, pd->corners);
			}
			break;
			case PRIMITIVE_TYPE::CIRCLE: {
				auto pd = (pc_circle*)&*prim.pdata;
				pd->point.compute(size);
				pd->radius.compute(size_avg);
				gfx2d::point_compute_circle<compute_only_draw_style>::compute_and_draw(pd->point.value + offset,
																					   pd->radius.value,
																					   pd->radius.value);
			}
			break;
			case PRIMITIVE_TYPE::CIRCLE_SECTOR: {
				auto pd = (pc_circle_sector*)&*prim.pdata;
				pd->point.compute(size);
				pd->radius.compute(size_avg);
				pd->start_angle.compute(size_avg);
				pd->end_angle.compute(size_avg);
				gfx2d::point_compute_circle_sector<compute_only_draw_style>::compute_and_draw(pd->point.value + offset,
																							  pd->radius.value,
																							  pd->radius.value,
																							  pd->start_angle.value,
																							  pd->end_angle.value);
			}
			break;
			case PRIMITIVE_TYPE::ELLIPSOID: {
				auto pd = (pc_ellipsoid*)&*prim.pdata;
				pd->point.compute(size);
				pd->radius_lr.compute(size.x);
				pd->radius_tb.compute(size.y);
				gfx2d::point_compute_circle<compute_only_draw_style>::compute_and_draw(pd->point.value + offset,
																					   pd->radius_lr.value,
																					   pd->radius_tb.value);
			}
			break;
			case PRIMITIVE_TYPE::ELLIPSOID_SECTOR: {
				auto pd = (pc_ellipsoid_sector*)&*prim.pdata;
				pd->point.compute(size);
				pd->radius_lr.compute(size.x);
				pd->radius_tb.compute(size.y);
				pd->start_angle.compute(size_avg);
				pd->end_angle.compute(size_avg);
				gfx2d::point_compute_circle_sector<compute_only_draw_style>::compute_and_draw(pd->point.value + offset,
																							  pd->radius_lr.value,
																							  pd->radius_tb.value,
																							  pd->start_angle.value,
																							  pd->end_angle.value);
			}
			break;
			case PRIMITIVE_TYPE::TEXT: {
				auto pd = (pc_text*)&*prim.pdata;
				auto ds = (ds_text*)&*prim.ddata;
				
				// cache the text (+compute its extent)
				const string text(text_lookup(pd->identifier));
				const auto cache_iter = text_cache.find(text);
				font::text_cache text_data;
				if(cache_iter != text_cache.cend()) {
					text_data = cache_iter->second;
				}
				else {
					text_data = fnt->cache_text(text);
					const auto x_advance_map(fnt->compute_advance_map(text, 0));
					const auto y_advance_map(fnt->compute_advance_map(text, 1));
					float y_alignment = 0.0f;
					if(y_advance_map.back().x < y_advance_map.back().y) {
						// single line
						// -> center in itself
						y_alignment = (y_advance_map.back().y - y_advance_map.back().x) * 0.5f;
						// -> text offset
						y_alignment += y_advance_map.back().x;
					}
					else {
						// multi-line
						y_alignment = (y_advance_map.back().x + y_advance_map.back().y) * 0.5f;
					}
					text_data.second.set(x_advance_map.back().x, y_alignment);
					text_cache.insert(make_pair(text, text_data));
				}
				
				//
				pd->position.compute(size);
				ds->color.compute(scheme);
				
				// handle positioning (centering!)
				float2 position(pd->position.value + offset);
				if(pd->position.ui_value[0].type == VALUE_TYPE::CENTER) {
					position.x -= text_data.second.x * 0.5f;
				}
				if(pd->position.ui_value[1].type == VALUE_TYPE::CENTER) {
					position.y -= text_data.second.y;
				}
				
				// floor position, so we don't get any draw artificats due to float imprecision
				position.floor();
				
				//
				fnt->draw_cached(text_data.first.first, text_data.first.second,
								 position,  ds->color.value);
				continue; // nothing more to do here
			}
		}
		
		// draw
		switch(prim.style) {
			//
			case DRAW_STYLE::BORDER_FILL:
				((ds_border_fill*)&*prim.ddata)->thickness.compute(size_avg);
#if defined(__clang__)
				[[clang::fallthrough]];
#endif
			case DRAW_STYLE::FILL: {
				auto ds = (ds_fill*)&*prim.ddata;
				ds->color.compute(scheme);
				if(prim.style == DRAW_STYLE::FILL) {
					gfx2d::draw_style_fill::draw(pprops, ds->color.value);
				}
				else {
					gfx2d::draw_style_border<gfx2d::draw_style_fill>::draw(pprops, ((ds_border_fill*)&*prim.ddata)->thickness.value, ds->color.value);
				}
			}
			break;
			
			//
			case DRAW_STYLE::BORDER_GRADIENT:
				((ds_border_gradient*)&*prim.ddata)->thickness.compute(size_avg);
#if defined(__clang__)
			[[clang::fallthrough]];
#endif
			case DRAW_STYLE::GRADIENT: {
				auto ds = (ds_gradient*)&*prim.ddata;
				vector<float4> colors;
				for(auto& color : ds->colors) {
					color.compute(scheme);
					colors.emplace_back(color.value);
				}
				if(prim.style == DRAW_STYLE::GRADIENT) {
					gfx2d::draw_style_gradient::draw(pprops, ds->type, ds->stops, colors);
				}
				else {
					gfx2d::draw_style_border<gfx2d::draw_style_gradient>::draw(pprops, ((ds_border_gradient*)&*prim.ddata)->thickness.value,
																			   ds->type, ds->stops, colors);
				}
			}
			break;
			
			//
			case DRAW_STYLE::BORDER_TEXTURE:
				((ds_border_texture*)&*prim.ddata)->thickness.compute(size_avg);
#if defined(__clang__)
				[[clang::fallthrough]];
#endif
			case DRAW_STYLE::TEXTURE: {
				auto ds = (ds_texture*)&*prim.ddata;
				ds->mul_color.compute(scheme);
				ds->add_color.compute(scheme);
				
				// external/dynamic texture: lookup texture num
				if(!ds->texture_name.empty()) {
					ds->texture = texture_lookup(ds->texture_name);
					if(ds->texture == 0) {
						oclr_error("texture \"%s\" not found!", ds->texture_name);
						ds->texture = texture_manager::get_dummy_texture();
					}
				}
				
				if(!ds->is_gradient) {
					if(ds->passthrough) {
						if(prim.style == DRAW_STYLE::TEXTURE) {
							gfx2d::draw_style_texture::draw(pprops, *ds->texture,
															true, ds->bottom_left, ds->top_right, ds->depth);
						}
						else {
							gfx2d::draw_style_border<gfx2d::draw_style_texture>::draw(pprops, ((ds_border_texture*)&*prim.ddata)->thickness.value,
																					  *ds->texture,  true, ds->bottom_left, ds->top_right, ds->depth);
						}
					}
					else {
						if(prim.style == DRAW_STYLE::TEXTURE) {
							gfx2d::draw_style_texture::draw(pprops, *ds->texture,
															ds->mul_color.value, ds->add_color.value,
															ds->bottom_left, ds->top_right, ds->depth);
						}
						else {
							gfx2d::draw_style_border<gfx2d::draw_style_texture>::draw(pprops, ((ds_border_texture*)&*prim.ddata)->thickness.value,
																					  *ds->texture, ds->mul_color.value, ds->add_color.value,
																					  ds->bottom_left, ds->top_right, ds->depth);
						}
					}
				}
				else {
					vector<float4> colors;
					for(auto& color : ds->gradient.colors) {
						color.compute(scheme);
						colors.emplace_back(color.value);
					}
					if(prim.style == DRAW_STYLE::TEXTURE) {
						gfx2d::draw_style_texture::draw(pprops, *ds->texture,
														ds->mul_color.value, ds->add_color.value,
														ds->gradient.type, ds->gradient.stops, colors,
														ds->gradient_mul_interpolator,
														ds->gradient_add_interpolator,
														ds->bottom_left, ds->top_right, ds->depth);
					}
					else {
						gfx2d::draw_style_border<gfx2d::draw_style_texture>::draw(pprops, ((ds_border_texture*)&*prim.ddata)->thickness.value,
																				  *ds->texture, ds->mul_color.value, ds->add_color.value,
																				  ds->gradient.type, ds->gradient.stops, colors,
																				  ds->gradient_mul_interpolator,
																				  ds->gradient_add_interpolator,
																				  ds->bottom_left, ds->top_right, ds->depth);
					}
				}
			}
			break;
			case DRAW_STYLE::TEXT: break;
		}
	}
	
	// reset scissor rect
	if(scissor) {
		glScissor(0, 0, oclraster::get_width(), oclraster::get_height());
	}
}

void gui_theme::ui_float::compute(const float& size) {
	switch(type) {
		case VALUE_TYPE::PIXEL:
			// just clamp to size
			value = std::min(ui_value, size);
			break;
		case VALUE_TYPE::PERCENTAGE:
			// scale by size
			value = ui_value * size;
			break;
		case VALUE_TYPE::POINT:
			// convert point to pixel (-> multiply by dpi / 72)
			value = point_to_pixel(ui_value);
			break;
		case VALUE_TYPE::CENTER:
			// scale by size
			value = 0.5f * size;
			break;
	}
}
