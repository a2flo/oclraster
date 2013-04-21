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

#include "font_manager.h"

#include <ft2build.h>
#include FT_FREETYPE_H

font_manager::font_manager() :
thread_base("font_manager"),
fonts(), ft_library(nullptr) {
	// init freetype
	if(FT_Init_FreeType(&ft_library) != 0) {
		oclr_error("failed to initialize freetype library!");
	}
	
	// these should always exist
	add_font_family("DEJAVU_SANS_SERIF", vector<string> {
		oclraster::data_path("fonts/DejaVuSans.ttf"),
		oclraster::data_path("fonts/DejaVuSans-Oblique.ttf"),
		oclraster::data_path("fonts/DejaVuSans-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSans-BoldOblique.ttf")
	});
	add_font_family("DEJAVU_SERIF", vector<string> {
		oclraster::data_path("fonts/DejaVuSerif.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-Italic.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-BoldItalic.ttf")
	});
	add_font_family("DEJAVU_MONOSPACE", vector<string> {
		oclraster::data_path("fonts/DejaVuSansMono.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-Oblique.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-BoldOblique.ttf")
	});
	
	// TODO: add system font overrides to config
#if defined(__APPLE__)
	add_font("SYSTEM_SANS_SERIF", "/System/Library/Fonts/LucidaGrande.ttc");
	add_font("SYSTEM_SERIF", "/System/Library/Fonts/Times.dfont");
	add_font("SYSTEM_MONOSPACE", "/System/Library/Fonts/Menlo.ttc");
#elif defined(__WINDOWS__)
	// TODO: find correct windows path
	add_font_family("SYSTEM_SANS_SERIF", {
		"C:/Windows/Fonts/arial.ttf",
		"C:/Windows/Fonts/ariali.ttf",
		"C:/Windows/Fonts/arialbd.ttf",
		"C:/Windows/Fonts/arialbi.ttf"
	});
	add_font_family("SYSTEM_SERIF", {
		"C:/Windows/Fonts/times.ttf",
		"C:/Windows/Fonts/timesi.ttf",
		"C:/Windows/Fonts/timesbd.ttf",
		"C:/Windows/Fonts/timesbi.ttf"
	});
	add_font_family("SYSTEM_MONOSPACE", {
		"C:/Windows/Fonts/cour.ttf",
		"C:/Windows/Fonts/couri.ttf",
		"C:/Windows/Fonts/courbd.ttf",
		"C:/Windows/Fonts/courbi.ttf"
	});
#else // linux/*bsd/x11
	// there isn't much choice here ...
	add_font_family("SYSTEM_SANS_SERIF", vector<string> {
		oclraster::data_path("fonts/DejaVuSans.ttf"),
		oclraster::data_path("fonts/DejaVuSans-Oblique.ttf"),
		oclraster::data_path("fonts/DejaVuSans-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSans-BoldOblique.ttf")
	});
	add_font_family("SYSTEM_SERIF", vector<string> {
		oclraster::data_path("fonts/DejaVuSerif.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-Italic.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSerif-BoldItalic.ttf")
	});
	add_font_family("SYSTEM_MONOSPACE", vector<string> {
		oclraster::data_path("fonts/DejaVuSansMono.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-Oblique.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-Bold.ttf"),
		oclraster::data_path("fonts/DejaVuSansMono-BoldOblique.ttf")
	});
#endif
	
	// start actual font_manager thread after everything has been initialized
	this->set_thread_delay(50);
	this->start();
}

font_manager::~font_manager() {
	oclr_debug("deleting font_manager object");
	
	// delete all fonts
	for(const auto& fnt : fonts) {
		delete fnt.second;
	}
	fonts.clear();
	
	// free freetype
	if(FT_Done_FreeType(ft_library) != 0) {
		oclr_error("failed to free freetype library!");
	}

	oclr_debug("font_manager object deleted");
}

void font_manager::run() {
}

FT_Library font_manager::get_ft_library() {
	return ft_library;
}

font& font_manager::add_font(const string& identifier, const string& filename) {
	return add_font_family(identifier, vector<string> { filename });
}

font& font_manager::add_font_family(const string& identifier, const vector<string> filenames) {
	const auto iter = fonts.find(identifier);
	if(iter != fonts.end()) {
		return *iter->second;
	}
	
	const auto ret = fonts.emplace(identifier, new font(this, filenames));
	return *ret.first->second;
}

bool font_manager::remove_font(const string& identifier) {
	const auto iter = fonts.find(identifier);
	if(iter == fonts.end()) {
		oclr_error("invalid font %s!", identifier);
		return false;
	}
	
	delete iter->second;
	fonts.erase(iter);
	return true;
}

font* font_manager::get_font(const string& identifier) const {
	const auto iter = fonts.find(identifier);
	if(iter != fonts.end()) {
		return iter->second;
	}
	return nullptr;
}
