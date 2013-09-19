/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
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

#include "texman.hpp"
#include <oclraster/oclraster.hpp>
#include <oclraster/pipeline/image.hpp>

pipeline* texture_manager::oclr_pipeline = nullptr;
image* texture_manager::dummy_texture = nullptr;
unordered_map<string, image*> texture_manager::images;

void texture_manager::init(pipeline* p_) {
	oclr_pipeline = p_;
	
	array<unsigned int, 4> dummy_pixels {{ 0, 0, 0, 0 }};
	dummy_texture = new image(2, 2, image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA, &dummy_pixels[0]);
}

void texture_manager::destroy() {
	if(dummy_texture != nullptr) delete dummy_texture;
	for(auto& img : images) {
		delete img.second;
	}
	images.clear();
}

image* texture_manager::get_dummy_texture() {
	return dummy_texture;
}

image* texture_manager::add_texture(const string& filename) {
	const auto img_iter = images.find(filename);
	if(img_iter != images.end()) {
		// image already exists
		return (img_iter->second->is_valid() ? img_iter->second : dummy_texture);
	}
	
	image* img = new image(image::from_file(filename, image::BACKING::IMAGE, IMAGE_TYPE::UINT_8, IMAGE_CHANNEL::RGBA));
	images.emplace(filename, img);
	return (img->is_valid() ? img : dummy_texture);
}
