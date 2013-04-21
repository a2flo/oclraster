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

#include "gui_item_container.h"
#include "gui_event.h"

gui_item_container::gui_item_container(const float2& size_, const float2& position_, const GUI_EVENT select_event_) :
gui_object(size_, position_), select_event(select_event_) {
}

gui_item_container::~gui_item_container() {
}

void gui_item_container::clear() {
	display_items.clear();
	items.clear();
	redraw();
}

void gui_item_container::add_item(const string& identifier, const string& label) {
	if(items.count(identifier) > 0) {
		oclr_error("an item with the identifier \"%s\" already exists!", identifier);
		return;
	}
	const auto iter = items.insert(make_pair(identifier, label));
	display_items.push_back(&*iter.first);
	if(display_items.size() == 1) {
		// set selected item and redraw, if this is the first item
		selected_item = display_items[0];
		redraw();
	}
}

void gui_item_container::remove_item(const string& identifier) {
	const auto iter = items.find(identifier);
	const auto disp_iter = find(begin(display_items), end(display_items), &*iter);
	
	if(iter == items.end()) {
		oclr_error("no item with the identifier \"%s\" found!", identifier);
		return;
	}
	if(selected_item == &*iter) {
		selected_item = nullptr;
		redraw(); // redraw, if this was the selected item
	}
	items.erase(iter);
	
	if(disp_iter == end(display_items)) {
		oclr_error("display item for identifier \"%s\" not found!", identifier);
	}
	else display_items.erase(disp_iter);
	
	if(selected_item == nullptr && !display_items.empty()) {
		selected_item = display_items[0];
	}
}

const pair<const string, string>* gui_item_container::get_selected_item() const {
	return selected_item;
}

void gui_item_container::set_selected_item(const string& identifier, const bool event_on_equal) {
	const auto iter = items.find(identifier);
	if(iter == items.end()) {
		oclr_error("no item with the identifier \"%s\" found!", identifier);
		return;
	}
	if(selected_item != &*iter || event_on_equal) {
		handle(select_event);
	}
	selected_item = &*iter;
}

void gui_item_container::set_selected_item(const size_t& index, const bool event_on_equal) {
	if(index >= display_items.size()) {
		oclr_error("index \"%u\" is greater than the amount of items!", index);
		return;
	}
	if(selected_item != display_items[index] || event_on_equal) {
		handle(select_event);
	}
	selected_item = display_items[index];
}
