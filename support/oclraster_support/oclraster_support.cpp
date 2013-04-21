/*
 *  Flexible OpenCL Rasterizer (oclraster_support)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "oclraster_support.h"
#include "oclraster_support_version.h"
#include "rendering/gfx2d.h"
#include "rendering/shader.h"
#include "rendering/texman.h"
#include "gui/gui.h"

// init statics
pipeline* oclraster_support::oclr_pipeline = nullptr;
gui* oclraster_support::ui = nullptr;

void oclraster_support::init(pipeline* oclr_pipeline_) {
	// print out oclraster_support info
	oclr_debug("%s", OCLRASTER_SUPPORT_VERSION_STRING);
	
	oclr_pipeline = oclr_pipeline_;
	shader_helper::init(oclr_pipeline_);
	gfx2d::init(oclr_pipeline_);
	texture_manager::init(oclr_pipeline_);
	
	ui = new gui("standard");
}

void oclraster_support::destroy() {
	if(ui != nullptr) delete ui;
	gfx2d::destroy();
	texture_manager::destroy();
	shader_helper::destroy();
}

pipeline* oclraster_support::get_pipeline() {
	return oclr_pipeline;
}

gui* oclraster_support::get_gui() {
	return ui;
}
