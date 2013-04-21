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

#include "gui_object.h"
#include "gui/gui.h"
#include "threading/task.h"
#include "font_manager.h"
#include "oclraster_support.h"

gui_object::gui_object(const float2& size_, const float2& position_) :
ui(oclraster_support::get_gui()), theme(ui->get_theme()), evt(oclraster::get_event()), fm(ui->get_font_manager()), fnt(fm->get_font("SYSTEM_SANS_SERIF")), size(size_), position(position_) {
	compute_abs_values();
}

gui_object::~gui_object() {
	// if this is the active gui object, set the active object to nullptr
	if(state.active || ui->get_active_object() == this) {
		ui->set_active_object(nullptr);
	}
	// remove from parent
	if(parent != nullptr) {
		parent->remove_child(this);
	}
}

bool gui_object::handle_draw() {
	if(!state.visible) return false;
	state.redraw = false;
	return true;
}

void gui_object::redraw() {
	state.redraw = true;
}

bool gui_object::needs_redraw() const {
	return state.redraw;
}

void gui_object::set_visible(const bool& visible_state) {
	state.visible = visible_state;
}

void gui_object::set_enabled(const bool& enabled_state) {
	state.enabled = enabled_state;
}

void gui_object::set_active(const bool& active_state) {
	state.active = active_state;
	
	// in case the active state is set externally (not by an event),
	// we have to make sure the gui knows about this
	if((state.active && ui->get_active_object() != this) ||
	   (!state.active && ui->get_active_object() == this)) {
		ui->set_active_object(state.active ? this : nullptr);
	}
	
	redraw();
}

bool gui_object::is_visible() const {
	return state.visible;
}

bool gui_object::is_enabled() const {
	return state.enabled;
}

bool gui_object::is_active() const {
	return state.active;
}

void gui_object::compute_abs_values() {
	const float2 parent_size(parent != nullptr ?
							 parent->get_size_abs() :
							 float2(oclraster::get_width(), oclraster::get_height()));
	position_abs = position * parent_size;
	size_abs = size * parent_size;
	rectangle_abs.set(position_abs.x, position_abs.y,
					  position_abs.x + size_abs.x, position_abs.y + size_abs.y);
}

const float2& gui_object::get_position() const {
	return position;
}

const float2& gui_object::get_position_abs() const {
	return position_abs;
}

const float2& gui_object::get_size() const {
	return size;
}

const float2& gui_object::get_size_abs() const {
	return size_abs;
}

const rect& gui_object::get_rectangle_abs() const {
	return rectangle_abs;
}

void gui_object::set_position(const float2& position_) {
	position = position_;
	compute_abs_values();
	redraw();
}

void gui_object::set_size(const float2& size_) {
	size = size_;
	compute_abs_values();
	redraw();
}

void gui_object::set_parent(gui_object* parent_) {
	lock();
	if(parent != nullptr) {
		parent->remove_child(this);
	}
	parent = parent_;
	compute_abs_values();
	unlock();
}

gui_object* gui_object::get_parent() const {
	return parent;
}

void gui_object::add_child(gui_object* child) {
	lock();
	children.insert(child);
	child->set_parent(this);
	unlock();
}

void gui_object::remove_child(gui_object* child) {
	lock();
	const auto iter = children.find(child);
	if(iter != children.end()) {
		children.erase(iter);
		child->set_parent(nullptr);
	}
	unlock();
}

const set<gui_object*>& gui_object::get_children() const {
	return children;
}

bool gui_object::handle_mouse_event(const EVENT_TYPE& type oclr_unused, const shared_ptr<event_object>& obj oclr_unused, const ipnt& point oclr_unused) {
	return false;
}

bool gui_object::handle_key_event(const EVENT_TYPE& type oclr_unused, const shared_ptr<event_object>& obj oclr_unused) {
	return false;
}

ipnt gui_object::abs_to_rel_position(const ipnt& point) const {
	// override this in objects that contain other objects (e.g. gui_window)
	if(parent != nullptr) return parent->abs_to_rel_position(point);
	return point;
}

ipnt gui_object::rel_to_abs_position(const ipnt& point) const {
	// override this in objects that contain other objects (e.g. gui_window)
	if(parent != nullptr) return parent->rel_to_abs_position(point);
	return point;
}

void gui_object::lock() {
	mutex.lock();
}

bool gui_object::try_lock() {
	return mutex.try_lock();
}

void gui_object::unlock() {
	mutex.unlock();
}

void gui_object::add_handler(handler&& handler_, GUI_EVENT type) {
	lock();
	handlers.insert(pair<GUI_EVENT, handler>(type, handler_));
	unlock();
}

void gui_object::handle(const GUI_EVENT gui_evt) {
	lock();
	const auto range = handlers.equal_range(gui_evt);
	for(auto iter = range.first; iter != range.second; iter++) {
		// we need a ref to the actual object, since we can't capture an iterator that
		// will be invalid/different after the next iteration
		const auto& hndlr = *iter;
		task::spawn([hndlr, gui_evt, this]() {
			hndlr.second(gui_evt, *this);
		});
	}
	unlock();
	
	// also add this event to the global event handler
	evt->add_event((EVENT_TYPE)gui_evt, make_shared<gui_event>(SDL_GetTicks(), *this));
}

void gui_object::remove_handlers(const GUI_EVENT& type) {
	lock();
	handlers.erase(type);
	unlock();
}

void gui_object::remove_handlers() {
	lock();
	handlers.clear();
	unlock();
}

bool gui_object::should_handle_mouse_event(const EVENT_TYPE& type oclr_unused, const ipnt& point) const {
	return gfx2d::is_pnt_in_rectangle(get_rectangle_abs(), point);
}
