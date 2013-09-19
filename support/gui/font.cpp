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

#include "font.hpp"
#include "font_manager.hpp"
#include "core/vector2.hpp"
#include "core/vector3.hpp"
#include "core/unicode.hpp"
#include "core/event_objects.hpp"
#include "rendering/shader.hpp"
#include <numeric>
#include <oclraster/oclraster.hpp>
#include <oclraster_support/oclraster_support.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

constexpr unsigned int font::font_texture_size;
constexpr size_t font_max_ubo_size = 32768; // constant memory is at least 64k -> since also other stuff is stored in there, use half

oclraster_struct font_tp_uniforms {
	matrix4f modelview_matrix;
	uint2 glyph_count; // (glyphs per row, glyphs per column)
	float2 glyph_size; // "font size"
	float2 page_size; // "texture size"
};

font::font(font_manager* fm_, const string& filename_) : font(fm_, vector<string> { filename_ }) {
}

font::font(font_manager* fm_, const vector<string> filenames_) :
fm(fm_), filenames(filenames_)
{
	set_size(10);
	
	// load fonts
	for(const auto& filename : filenames) {
		// load first face of file
		FT_Face first_face = nullptr;
		if(FT_New_Face(fm->get_ft_library(), filename.c_str(), 0, &first_face) != 0) {
			log_error("couldn't load font %s!", filename);
			return;
		}
		if(font_name == "NONE") font_name = first_face->family_name;
		const size_t num_faces = first_face->num_faces;
		
		// add if style doesn't exist yet
		if(!add_face(first_face->style_name, first_face)) {
			first_face = nullptr;
		}
		
		// load other faces if there are any and set properties
		for(size_t face_index = 0; face_index < num_faces; face_index++) {
			FT_Face face = nullptr;
			if(face_index > 0) {
				if(FT_New_Face(fm->get_ft_library(), filename.c_str(), face_index, &face) != 0) {
					log_error("couldn't load face #%u for font %s!", face_index, filename);
					return;
				}
				// add if style doesn't exist yet
				if(!add_face(face->style_name, face)) {
					face = nullptr;
				}
			}
			else {
				face = first_face;
			}
			
			if(face != nullptr) {
				if(FT_Set_Char_Size(face, 0, font_size * 64, 0, (FT_UInt)floor::get_dpi()) != 0) {
					log_error("couldn't set char size for face #%u in font %s!", face_index, filename);
					return;
				}
			}
		}
	}
	
	// make sure we have a font for each style (if not, use another one as fallback)
	if(faces.empty()) {
		log_error("couldn't load any faces for font %s!", font_name);
		return;
	}
	
	if(faces.find("Regular") == faces.end()) {
		// use any since there is no real fallback
		faces.emplace("Regular", faces.cbegin()->second);
	}
	if(faces.find("Bold Italic") == faces.end()) {
		if(faces.find("Bold") != faces.end()) {
			faces.emplace("Bold Italic", faces.find("Bold")->second);
		}
		if(faces.find("Italic") != faces.end()) {
			faces.emplace("Bold Italic", faces.find("Italic")->second);
		}
		else {
			faces.emplace("Bold Italic", faces.find("Regular")->second);
		}
	}
	if(faces.find("Italic") == faces.end()) {
		faces.emplace("Italic", faces.find("Regular")->second);
	}
	if(faces.find("Bold") == faces.end()) {
		faces.emplace("Bold", faces.find("Regular")->second);
	}
	
	// create glyph maps
	for(const auto& face : faces) {
		glyph_map.emplace(face.first, decltype(glyph_map)::mapped_type());
	}
	
	// cache most used glyphs
	cache(BMP_BLOCK::BASIC_LATIN);
	cache(BMP_BLOCK::LATIN_1_SUPPLEMENT);
	
	// create necessary glyph vbo and ubo
	const float4 glyph_quad[] {
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 0.0f, 0.0f, 1.0f }
	};
	glyph_vbo = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
								   opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
								   opencl::BUFFER_FLAG::INITIAL_COPY,
								   4 * sizeof(float4), &glyph_quad[0]);
	
	text_ubo = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
								  opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
								  font_max_ubo_size);
	
	tp_uniforms = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
									 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
									 sizeof(font_tp_uniforms));
	
	// only temporary
	array<unsigned int, 4> indices {{ 0, 1, 2, 3 }};
	_tmp_indices = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
									  opencl::BUFFER_FLAG::BLOCK_ON_WRITE |
									  opencl::BUFFER_FLAG::INITIAL_COPY,
									  sizeof(unsigned int) * 4,
									  &indices[0]);
}

font::~font() {
	// different styles may point to the same FT_Font -> create a set
	set<FT_Face> ft_faces;
	for(const auto& face : faces) {
		ft_faces.emplace(face.second);
	}
	for(const auto& face : ft_faces) {
		if(FT_Done_Face(face) != 0) {
			log_error("failed to free face for font %s!", font_name);
		}
	}
	
	if(glyph_vbo != nullptr) ocl->delete_buffer(glyph_vbo);
	if(text_ubo != nullptr) ocl->delete_buffer(text_ubo);
	if(tp_uniforms != nullptr) ocl->delete_buffer(tp_uniforms);
	if(tex_array != nullptr) delete tex_array;
}

bool font::add_face(const string& style, FT_Face face) {
	// style remapping
	auto map_style = [](string style_str) -> string {
		static const unordered_map<string, string> style_remapping {
			{ "Roman", "Regular" },
			{ "Book", "Regular" },
			{ "Oblique", "Italic" },
			{ "Bold Oblique", "Bold Italic" },
			{ "R", "Regular" },
			{ "I", "Italic" },
			{ "O", "Italic" },
			{ "B", "Bold" },
			{ "BI", "Bold Italic" },
			{ "BO", "Bold Italic" },
			{ "Medium", "Regular" },
			{ "M", "Regular" },
			{ "Medium Italic", "Italic" },
			{ "MI", "Italic" },
			{ "BoldItalic", "Bold Italic" },
			{ "BoldOblique", "Bold Italic" },
		};
		const auto iter = style_remapping.find(style_str);
		return (iter == style_remapping.end() ? style_str : iter->second);
	};
	
	const string style_name(map_style(style));
	if(faces.find(style_name) == faces.end()) {
		faces.emplace(style_name, face);
		return true;
	}
	
	FT_Done_Face(face);
	return false;
}

bool font::is_cached(const unsigned int& code) const {
	for(const auto& face : faces) {
		const auto& glyphs = glyph_map.at(face.first);
		if(glyphs.find(code) == glyphs.end()) {
			// glyph has not been cached
			return false;
		}
	}
	return true;
}

void font::cache(const BMP_BLOCK block) {
	cache(((unsigned int)block >> 16) & 0xFFFF, (unsigned int)block & 0xFFFF);
}

void font::cache(const string& characters) {
	const vector<unsigned int> codes(unicode::utf8_to_unicode(characters));
	for(const auto& code : codes) {
		cache(code, code);
	}
}

void font::cache(const unsigned int& start_code, const unsigned int& end_code) {
	if(start_code > end_code) {
		log_error("start_code(%u) > end_code(%u)", start_code, end_code);
		return;
	}
	if(start_code > 0x10FFFF || end_code > 0x10FFFF) {
		log_error("invalid start_code(%u) or end_code(%u) - >0x10FFFF!", start_code, end_code);
		return;
	}
	
	// check if glyphs were already cached
	bool all_cached = true;
	for(unsigned int code = start_code; code < end_code+1; code++) {
		if(!is_cached(code)) {
			all_cached = false;
			break;
		}
	}
	if(all_cached) return;
	
	// check if we need to expand the texture to fit all glyphs in
	const size_t new_glyph_count = (end_code - start_code + 1) * faces.size();
	const size_t cur_glyph_count = accumulate(cbegin(glyph_map), cend(glyph_map), 0,
											  [](const size_t& accu, const decltype(glyph_map)::value_type& elem) {
												  return accu + elem.second.size(); // TODO: when glyph_data will contain more than one glyph, this needs to be computed here
	});
	if((cur_glyph_count + new_glyph_count) > (tex_array_layers * glyphs_per_layer)) {
		recreate_texture_array(((cur_glyph_count + new_glyph_count) / glyphs_per_layer) + 1);
	}
	
	// render glyph bitmaps and copy them to the texture
	size_t glyph_counter = cur_glyph_count;
	for(const auto& face : faces) {
		auto& glyphs = glyph_map[face.first];
		FT_GlyphSlot slot = face.second->glyph;
		for(unsigned int code = start_code; code < end_code+1; code++) {
			if(glyphs.find(code) != glyphs.end()) {
				// glyph has already been cached
				continue;
			}
			
			if(FT_Load_Char(face.second, code, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT | FT_LOAD_TARGET_LCD) != 0) {
				log_error("couldn't cache character %X!", code);
				continue;
			}
			
			//
			const unsigned int texture_index = (unsigned int)glyph_counter++;
			glyphs.emplace(code,
						  glyph_data {
							  texture_index,
							  int4(slot->bitmap_left,
								   int(display_font_size) - slot->bitmap_top,
								   (int)slot->advance.x,
								   (int)slot->advance.y),
							  int2(int(slot->metrics.width >> 6), int(slot->metrics.height >> 6))
						  });
			const unsigned int layer = texture_index / glyphs_per_layer;
			const int3 offset((texture_index - (layer*glyphs_per_layer)) % glyphs_per_line,
							  (texture_index - (layer*glyphs_per_layer)) / glyphs_per_line,
							  layer);
			
			// ignore 0px writes ...
			if(slot->bitmap.width == 0 || slot->bitmap.rows == 0) continue;
			
			// copy glyph bitmap rows to a correctly sized bitmap (display_font_size^2 or max glyph size^2),
			// this is necessary, because of row padding "provided" by freetype
			const size_t glyph_mem_size(display_font_size * display_font_size * 4);
			unsigned char* glyph_buffer = new unsigned char[glyph_mem_size];
			memset(glyph_buffer, 0, glyph_mem_size);
			
			const size_t row_data_length(display_font_size * 4);
			for(size_t row = 0; row < (size_t)slot->bitmap.rows; row++) {
				unsigned char* glyph_ptr = glyph_buffer + (row * row_data_length);
				unsigned char* bitmap_ptr = slot->bitmap.buffer + (row * slot->bitmap.pitch);
				for(size_t column = 0; column < (size_t)slot->bitmap.width; column+=3) {
					*glyph_ptr++ = *bitmap_ptr++;
					*glyph_ptr++ = *bitmap_ptr++;
					*glyph_ptr++ = *bitmap_ptr++;
					*glyph_ptr++ = 255u;
				}
			}
			
			tex_array->write(glyph_buffer,
							 { offset.x * display_font_size, offset.y * display_font_size }, // TODO: z = offset.z, when texture arrays are supported
							 { display_font_size, display_font_size }); // TODO: z = 1
			delete [] glyph_buffer;
		}
	}
}

font::text_cache font::cache_text(const string& text, opencl::buffer_object* existing_ubo) {
	//
	opencl::buffer_object* ubo = existing_ubo;
	if(ubo == nullptr) {
		ubo = ocl->create_buffer(opencl::BUFFER_FLAG::READ |
								 opencl::BUFFER_FLAG::BLOCK_ON_WRITE,
								 font_max_ubo_size);
	}
	
	// update ubo with text data
	const auto text_data(create_text_ubo_data(text, [&](unsigned int code) {
		this->cache(code, code);
	}));
	
	// TODO: handle overflow
	if(!text_data.first.empty()) {
		ocl->write_buffer(ubo, &text_data.first[0], 0, std::min(text_data.first.size() * sizeof(uint2), font_max_ubo_size));
	}
	
	return { { ubo, (unsigned int)text_data.first.size() }, text_data.second };
}

void font::destroy_text_cache(text_cache& cached_text) {
	if(cached_text.first.first != nullptr) {
		ocl->delete_buffer(cached_text.first.first);
		cached_text.first.first = nullptr;
	}
}

void font::recreate_texture_array(const size_t& layers) {
	const size_t prev_layers(tex_array_layers);
	
	// this works around an intel and ati driver bug (an array must at least have 2 layers)
	tex_array_layers = std::max(layers, (size_t)2);
	
	const size_t layer_size(font_texture_size * font_texture_size * 3);
	const size_t tex_size(layer_size * tex_array_layers);
	unsigned char* tex_data = new unsigned char[tex_size];
	
	// check if we need to copy data from the old tex layers
	if(prev_layers != 0 && tex_array != nullptr) {
		// TODO: implement this when texture arrays are supported
		log_error("2D texture arrays are not implemented yet!");
		return;
		
		/*tex_array->read(tex_data);
		
		const size_t layer_diff(tex_array_layers - prev_layers);
		memset(tex_data + prev_layers * layer_size, 0, layer_diff * layer_size); // clear new layers*/
	}
	else {
		memset(tex_data, 0, tex_size);
	}
	
	//
	if(tex_array == nullptr) {
		tex_array = new image(font_texture_size, font_texture_size, image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA, tex_data);
	}
	
	// TODO: see above
	/*glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB8,
				 font_texture_size, font_texture_size, (GLsizei)tex_array_layers,
				 0, GL_RGB, GL_UNSIGNED_BYTE, tex_data);*/
	
	delete [] tex_data;
}

void font::draw(const string& text, const float2& position, const float4 color) {
	const auto ubo = cache_text(text, text_ubo);
	draw_cached(text_ubo, ubo.first.second, position, color);
}

void font::draw_cached(const string& text, const float2& position, const float4 color) const {
	// update ubo with text data
	const auto text_data(create_text_ubo_data(text));
	if(text_data.first.empty()) return;
	
	// TODO: handle overflow
	ocl->write_buffer(text_ubo, &text_data.first[0], 0, std::max(text_data.first.size() * sizeof(uint2), font_max_ubo_size));
	
	draw_cached(text_ubo, text_data.first.size(), position, color);
}

pair<vector<uint2>, float2> font::create_text_ubo_data(const string& text,
													   std::function<void(unsigned int)> cache_fnc) const {
	vector<uint2> ubo_data;
	const float2 extent = text_stepper(text,
									   [&ubo_data](unsigned int code floor_unused,
												   const glyph_data& glyph,
												   const float2& origin floor_unused,
												   const float2& fpos) {
										   const int2 pos(fpos); // round to integer
										   ubo_data.emplace_back(glyph.tex_index,
																 (unsigned int)(((short int)pos.x) & 0xFFFFu) +
																 (((unsigned int)(((short int)pos.y) & 0xFFFFu)) << 16u));
									   },
									   [](unsigned int, const float2&, const float&){},
									   cache_fnc);
	return { ubo_data, extent };
}

float font::compute_advance(const string& str, const unsigned int component) const {
	return compute_advance(unicode::utf8_to_unicode(str), component);
}

float font::compute_advance(const vector<unsigned int>& unicode_str, const unsigned int component) const {
	const float2 extent = text_stepper(unicode_str);
	return extent[component];
}

vector<float2> font::compute_advance_map(const string& str, const unsigned int component) const {
	return compute_advance_map(unicode::utf8_to_unicode(str), component);
}

vector<float2> font::compute_advance_map(const vector<unsigned int>& unicode_str, const unsigned int component) const {
	vector<float2> advance_map;
	advance_map.reserve(unicode_str.size());
	float total_advance = 0.0f;
	int2 line_advance(numeric_limits<int>::max(), 0);
	text_stepper(unicode_str,
				 [&advance_map, &total_advance, &line_advance, &component]
				 (unsigned int code floor_unused,
				  const glyph_data& glyph,
				  const float2& origin floor_unused,
				  const float2& fpos floor_unused)
	{
		if(component == 0) { //x
			const float glyph_advance = float(glyph.layout.z >> 6);
			advance_map.emplace_back(float2(total_advance, glyph_advance));
			total_advance += glyph_advance;
		}
		else { // y
			advance_map.emplace_back(float2(total_advance + float(glyph.layout.z >> 6) + float(glyph.layout.y),
											float(glyph.size.y)));
			line_advance.x = std::min(line_advance.x, glyph.layout.w + glyph.layout.y);
			line_advance.y = std::max(line_advance.y, (glyph.layout.w + glyph.layout.y) + glyph.size.y);
		}
	},
				 [&total_advance, &line_advance, &component]
				 (unsigned int code floor_unused,
				  const float2& origin floor_unused,
				  const float& break_size)
	{
		if(component == 1) { // y
			total_advance += break_size;
			line_advance.set(numeric_limits<int>::max(), 0);
		}
	});
	// advance after the last character:
	if(component == 0) { // x
		advance_map.emplace_back(float2(total_advance, 0.0f));
	}
	if(component == 1) { // y
		if(line_advance.x != numeric_limits<int>::max()) {
			advance_map.emplace_back(float2(total_advance + line_advance.x, line_advance.y));
		}
		else advance_map.emplace_back(float2(total_advance, 0.0f));
	}
	return advance_map;
}

float2 font::text_stepper(const string& str,
						  std::function<void(unsigned int, const glyph_data&, const float2&, const float2&)> fnc,
						  std::function<void(unsigned int, const float2&, const float&)> line_break_fnc,
						  std::function<void(unsigned int)> cache_fnc) const {
	return text_stepper(unicode::utf8_to_unicode(str), fnc, line_break_fnc, cache_fnc);
}

float2 font::text_stepper(const vector<unsigned int>& unicode_str_,
						  std::function<void(unsigned int, const glyph_data&, const float2&, const float2&)> fnc,
						  std::function<void(unsigned int, const float2&, const float&)> line_break_fnc,
						  std::function<void(unsigned int)> cache_fnc) const {
	// replace control strings by control characters (easier to handle later on)
	static const struct {
		const vector<unsigned int> search;
		const unsigned int repl;
	} control_chars[] = {
		{ unicode::utf8_to_unicode(u8"<i>"), 1 },
		{ unicode::utf8_to_unicode(u8"</i>"), 2 },
		{ unicode::utf8_to_unicode(u8"<b>"), 3 },
		{ unicode::utf8_to_unicode(u8"</b>"), 4 },
	};
	auto unicode_str(unicode_str_); // copy!
	for(const auto& cc : control_chars) {
		auto iter = unicode_str.begin();
		while((iter = search(begin(unicode_str), end(unicode_str),
							 begin(cc.search), end(cc.search)))
			  != end(unicode_str)) {
			unicode_str.insert(unicode_str.erase(iter, iter + cc.search.size()), cc.repl);
		}
	}
	
	//
	float2 origin(0.0f);
	string cur_style = "Regular";
	
	const decltype(glyph_map)::value_type::second_type& regular_map(glyph_map.find("Regular")->second);
	const decltype(glyph_map)::value_type::second_type& italic_map(glyph_map.find("Italic")->second);
	const decltype(glyph_map)::value_type::second_type& bold_map(glyph_map.find("Bold")->second);
	const decltype(glyph_map)::value_type::second_type& bold_italic_map(glyph_map.find("Bold Italic")->second);
	const decltype(glyph_map)::value_type::second_type* cur_style_map = &regular_map;
	
	static const unsigned int tab_multiplier = 4;
	static const float leading_multiplier = 1.125f;
	const unordered_map<string, unordered_map<unsigned int, unsigned int>> whitespace_sizes {
		{
			"Regular", {
				{ 0x0A, float(display_font_size) * leading_multiplier },
				{ 0x0D, float(display_font_size) * leading_multiplier },
				{ 0x09, (regular_map.find(0x20)->second.layout.z >> 6) * tab_multiplier },
				{ 0x20, (regular_map.find(0x20)->second.layout.z >> 6) },
			}
		},
		{
			"Italic", {
				{ 0x0A, float(display_font_size) * leading_multiplier },
				{ 0x0D, float(display_font_size) * leading_multiplier },
				{ 0x09, (italic_map.find(0x20)->second.layout.z >> 6) * tab_multiplier },
				{ 0x20, (italic_map.find(0x20)->second.layout.z >> 6) },
			}
		},
		{
			"Bold", {
				{ 0x0A, float(display_font_size) * leading_multiplier },
				{ 0x0D, float(display_font_size) * leading_multiplier },
				{ 0x09, (bold_map.find(0x20)->second.layout.z >> 6) * tab_multiplier },
				{ 0x20, (bold_map.find(0x20)->second.layout.z >> 6) },
			}
		},
		{
			"Bold Italic", {
				{ 0x0A, float(display_font_size) * leading_multiplier },
				{ 0x0D, float(display_font_size) * leading_multiplier },
				{ 0x09, (bold_italic_map.find(0x20)->second.layout.z >> 6) * tab_multiplier },
				{ 0x20, (bold_italic_map.find(0x20)->second.layout.z >> 6) },
			}
		},
	};
	const auto whitespace_size = [&](const unsigned int code) -> unsigned int {
		return whitespace_sizes.find(cur_style)->second.find(code)->second;
	};
	
	float2 extent;
	bool style_italic = false, style_bold = false;
	for(const auto& code : unicode_str) {
		// handle control characters / whitespace
		switch(code) {
			case 0x01:
				style_italic = true;
				break;
			case 0x02:
				style_italic = false;
				break;
			case 0x03:
				style_bold = true;
				break;
			case 0x04:
				style_bold = false;
				break;
			case 0x0A:
			case 0x0D: {
				const float line_break_size = whitespace_size(0x0A);
				origin.y += line_break_size;
				origin.x = 0.0f;
				line_break_fnc(code, origin, line_break_size);
				continue;
			}
			case 0x09:
				origin.x += whitespace_size(0x09);
				continue;
			default: break;
		}
		
		//
		switch(code) {
			case 0x01:
			case 0x02:
			case 0x03:
			case 0x04: {
				const size_t style_idx = size_t(style_italic) + size_t(style_bold)*2;
				cur_style = vector<string> { "Regular", "Italic", "Bold", "Bold Italic" }[style_idx];
				cur_style_map = vector<decltype(cur_style_map)> { &regular_map, &italic_map, &bold_map, &bold_italic_map }[style_idx];
				continue;
			}
			default: break;
		}
		
		auto iter = cur_style_map->find(code);
		if(iter == cur_style_map->end()) {
			// cache before we continue
			cache_fnc(code);
			
			// try again
			iter = cur_style_map->find(code);
			if(iter == cur_style_map->end()) {
				// if it hasn't been cached, ignore it
				origin.x += whitespace_size(0x20);
				continue;
			}
		}
		
		const float2 fpos(origin.x + float(iter->second.layout.x), origin.y + float(iter->second.layout.y));
		if(code != 0x20) { // ignore spaces (other whitespace has already been ignored)
			extent = float2::max(fpos, extent); // max extent
		}
		
		fnc(code, iter->second, origin, fpos);
		
		origin.x += iter->second.layout.z >> 6;
		origin.y += iter->second.layout.w >> 6;
	}
	extent = float2::max(extent, origin); // in case of whitespace, do a last max
	return extent;
}

void font::draw_cached(const opencl::buffer_object* ubo, const size_t& character_count, const float2& position, const float4 color) const {
	if(ubo == nullptr || character_count == 0) return;
	
	// draw
	font_tp_uniforms transform_uniforms {
		matrix4f().translate(position.x, position.y, 0.0f),
		uint2(glyphs_per_line, glyphs_per_line),
		float2(display_font_size),
		float2(font_texture_size)
	};
	ocl->write_buffer(tp_uniforms, &transform_uniforms, 0, sizeof(font_tp_uniforms));
	
	pipeline* oclr_pipeline = oclraster_support::get_pipeline();
	shader_helper::font_shd->use("#", false, tp_uniforms);
	shader_helper::font_shd->set_uniforms(color);
	
	oclr_pipeline->bind_image("font_texture", *tex_array);
	oclr_pipeline->bind_buffer("text_data", *ubo);
	
	// note: ubo data size / character_count (actual) != codes size / #characters in text (wanted)
	oclr_pipeline->bind_buffer("index_buffer", *_tmp_indices);
	oclr_pipeline->bind_buffer("input_attributes", *glyph_vbo);
	
	oclr_pipeline->draw_instanced(PRIMITIVE_TYPE::TRIANGLE_STRIP, 4, { 0, 2 }, (unsigned int)character_count);
}

void font::set_size(const unsigned int& size) {
	font_size = size;
	// slightly weird, but it seems to work for misc dpi sizes
	display_font_size = (unsigned int)ceilf((float(font_size * floor::get_dpi()) / 64.0f) * (72.0f / 64.0f));
	glyphs_per_line = font_texture_size / display_font_size;
	glyphs_per_layer = glyphs_per_line * glyphs_per_line;
}

const unsigned int& font::get_size() const {
	return font_size;
}

const unsigned int& font::get_display_size() const {
	return display_font_size;
}

const vector<string> font::get_available_styles() const {
	vector<string> ret;
	for(const auto& face : faces) {
		ret.emplace_back(face.first);
	}
	return ret;
}

const image* font::_get_image() const {
	return tex_array;
}
