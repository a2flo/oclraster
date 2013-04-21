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

#include "gui_image.h"
#include "rendering/gfx2d.h"
#include "rendering/texman.h"

gui_image::gui_image() {
}

gui_image::~gui_image() {
}

void gui_image::draw(unsigned int scale_x, unsigned int scale_y, bool flip_y) {
	if(!gui_img) oclraster::start_2d_draw();
	
	const float4 fcolor(float((color>>16) & 0xFF) / 255.0f,
						float((color>>8) & 0xFF) / 255.0f,
						float(color & 0xFF) / 255.0f,
						1.0f);
	
	coord bottom_left, top_right;
	if(flip_y) {
		bottom_left.set(0.0f, 1.0f);
		top_right.set(1.0f, 0.0f);
	}
	else {
		bottom_left.set(0.0f, 0.0f);
		top_right.set(1.0f, 1.0f);
	}

	rect rectangle;
	if(scale) {
		rectangle.x1 = position.x;
		rectangle.y1 = position.y;
		rectangle.x2 = position.x + scale_x;
		rectangle.y2 = position.y + scale_y;
	}
	else {
		rectangle.x1 = position.x;
		rectangle.y1 = position.y;
		rectangle.x2 = position.x + tex->get_size().x;
		rectangle.y2 = position.y + tex->get_size().y;
	}
	gfx2d::draw_rectangle_texture(rectangle, *tex, fcolor, float4(0.0f), bottom_left, top_right);

	// if we want to draw 3d stuff later on, we have to clear
	// the depth buffer, otherwise nothing will be seen
	glClear(GL_DEPTH_BUFFER_BIT);

	if(!gui_img) oclraster::stop_2d_draw();
}

void gui_image::draw() {
	draw(tex->get_size().x, tex->get_size().y);
}

/*! opens an image file
 *  @param filename the image files name
 */
void gui_image::open_image(const char* filename) {
	tex = texture_manager::add_texture(filename);
}

/*! sets the position (2 * unsigned int) of the image
 *  @param x the (new) x position of the image
 *  @param y the (new) y position of the image
 */
void gui_image::set_position(unsigned int x, unsigned int y) {
	position.x = x;
	position.y = y;
}

/*! sets the position (pnt) of the image
 *  @param position the (new) position of the image
 */
void gui_image::set_position(pnt* position_) {
	set_position(position_->x, position_->y);
}

/*! returns the position (pnt) of the image
 */
pnt& gui_image::get_position() {
	return position;
}

/*! sets the images texture
 *  @param tex the texture we want to set
 */
void gui_image::set_texture(const image& new_tex) {
	tex = &new_tex;
}

//! returns the images texture
const image* gui_image::get_texture() const {
	return tex;
}

/*! sets image scaling to state
 *  @param state the scaling state
 */
void gui_image::set_scaling(bool state) {
	gui_image::scale = state;
}

//! returns the image scale flag
bool gui_image::get_scaling() {
	return scale;
}

unsigned int gui_image::get_width() {
	return tex->get_size().x;
}

unsigned int gui_image::get_height() {
	return tex->get_size().y;
}

void gui_image::set_color(unsigned int color_) {
	gui_image::color = color_;
}

void gui_image::set_gui_img(bool state) {
	gui_img = state;
}
