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

#ifndef __OCLRASTER_SUPPORT_FONT_HPP__
#define __OCLRASTER_SUPPORT_FONT_HPP__

#include "oclraster_support/global.hpp"
#include "core/event.hpp"
#include <floor/cl/opencl.hpp>
#include <oclraster/pipeline/image.hpp>

/*! @class font
 *  @brief stores a font and can be used for drawing
 */

class font_manager;
typedef struct FT_FaceRec_* FT_Face;
enum class EVENT_TYPE : unsigned int;
struct event_object;
class FLOOR_API font {
public:
	//! single font file or font collection
	font(font_manager* fm, const string& filename);
	//! multiple font files (note: only the first of each style will be used)
	font(font_manager* fm, const vector<string> filenames);
	~font();
	
	// draw functions
	void draw(const string& text, const float2& position, const float4 color = float4(1.0f));
	void draw_cached(const string& text, const float2& position, const float4 color = float4(1.0f)) const;
	void draw_cached(const opencl::buffer_object* ubo, const size_t& character_count, const float2& position, const float4 color = float4(1.0f)) const;
	
	// style functions
	const vector<string> get_available_styles() const;
	const unsigned int& get_size() const;
	const unsigned int& get_display_size() const;
	
	float compute_advance(const string& str, const unsigned int component = 0) const;
	float compute_advance(const vector<unsigned int>& unicode_str, const unsigned int component = 0) const;
	vector<float2> compute_advance_map(const string& str, const unsigned int component = 0) const;
	vector<float2> compute_advance_map(const vector<unsigned int>& unicode_str, const unsigned int component = 0) const;
	
	//! http://en.wikipedia.org/wiki/Basic_Multilingual_Plane#Basic_Multilingual_Plane
	enum class BMP_BLOCK : unsigned int {
		BASIC_LATIN				= 0x0000007F, // NOTE: will be cached automatically (0000 - 007F)
		LATIN_1_SUPPLEMENT		= 0x008000FF, // NOTE: will be cached automatically (0080 - 00FF)
		LATIN_EXTENDED_A		= 0x0100017F, // 0100 - 017F
		LATIN_EXTENDED_B		= 0x0180024F, // 0180 - 024F
		LATIN_IPA_EXTENSIONS	= 0x025002AF, // 0250 - 02AF
		GREEK					= 0x037003FF, // 0370 - 03FF
		CYRILLIC				= 0x040004FF, // 0400 - 04FF
		CYRILLIC_SUPPLEMENT		= 0x0500052F, // 0500 - 052F
		ARMENIAN				= 0x0530058F, // 0530 - 058F
		HEBREW					= 0x059005FF, // 0590 - 05FF
		ARABIC					= 0x060006FF, // 0600 - 06FF
		ARABIC_SUPPLEMENT		= 0x0750077F, // 0750 - 077F
		HIRAGANA				= 0x3040309F, // 3040 - 309F
		KATAKANA				= 0x30A030FF  // 30A0 - 30FF
	};
	//! shortcut for common blocks
	void cache(const BMP_BLOCK block);
	//! this must be utf-8 encoded
	void cache(const string& characters);
	//! should be within 0x0 - 0x10FFFF
	void cache(const unsigned int& start_code, const unsigned int& end_code);
	//! <<ubo, character_count>, extent>, note: ubo must be destroyed/managed manually!
	typedef pair<pair<opencl::buffer_object*, unsigned int>, float2> text_cache;
	text_cache cache_text(const string& text, opencl::buffer_object* existing_ubo = nullptr);
	static void destroy_text_cache(text_cache& cached_text);
	
	bool is_cached(const unsigned int& code) const;
	
	// TODO: clear cache
	//void clear_cache();
	
	// texture cache info
	static constexpr unsigned int font_texture_size = 2048;
	
	// unicode -> texture index
	struct glyph_data {
		const unsigned int tex_index;
		const int4 layout;
		const int2 size;
	};
	
	// for debugging purposes
	const image* _get_image() const;
	
protected:
	font_manager* fm = nullptr;
	const vector<string> filenames;
	string font_name = "NONE";
	
	// style -> ft face
	unordered_map<string, FT_Face> faces;
	bool add_face(const string& style, FT_Face face);
	
	// style -> (unicode -> glyph data)
	unordered_map<string, unordered_map<unsigned int, glyph_data>> glyph_map;
	unsigned int font_size = 10;
	unsigned int display_font_size = font_size;
	unsigned int glyphs_per_line = 0;
	unsigned int glyphs_per_layer = 0;
	
	void recreate_texture_array(const size_t& layers);
	image* tex_array = nullptr;
	size_t tex_array_layers = 0;
	opencl::buffer_object* glyph_vbo = nullptr;
	opencl::buffer_object* tp_uniforms = nullptr;
	opencl::buffer_object* _tmp_indices = nullptr;
	
	pair<vector<uint2>, float2> create_text_ubo_data(const string& text, std::function<void(unsigned int)> cache_fnc = [](unsigned int){}) const;
	opencl::buffer_object* text_ubo = nullptr;
	
	// size functions (TODO: will be made public when dynamic size changes are supported)
	void set_size(const unsigned int& size);
	
	//
	float2 text_stepper(const string& str,
						std::function<void(unsigned int, const glyph_data&, const float2&, const float2&)> fnc = [](unsigned int, const glyph_data&, const float2&, const float2&){},
						std::function<void(unsigned int, const float2&, const float&)> line_break_fnc = [](unsigned int, const float2&, const float&){},
						std::function<void(unsigned int)> cache_fnc = [](unsigned int){}) const;
	float2 text_stepper(const vector<unsigned int>& unicode_str,
						std::function<void(unsigned int, const glyph_data&, const float2&, const float2&)> fnc = [](unsigned int, const glyph_data&, const float2&, const float2&){},
						std::function<void(unsigned int, const float2&, const float&)> line_break_fnc = [](unsigned int, const float2&, const float&){},
						std::function<void(unsigned int)> cache_fnc = [](unsigned int){}) const;
	
};

#endif
