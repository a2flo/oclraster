/*
 *  Flexible OpenCL Rasterizer (oclraster)
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

#ifndef __OCLRASTER_HPP__
#define __OCLRASTER_HPP__

#include "oclraster/global.hpp"
#include "core/core.hpp"
#include "core/file_io.hpp"
#include "core/event.hpp"
#include "core/xml.hpp"
#include "core/vector3.hpp"
#include "core/matrix4.hpp"
#include "core/unicode.hpp"

class opencl_base;
extern opencl_base* ocl;

class pipeline;

class FLOOR_API oclraster {
public:
	static void init(const char* callpath_, const char* datapath_);
	static void destroy();
	
	static void start_draw();
	static void stop_draw();
	
	static void set_active_pipeline(pipeline* active_pipeline);
	static pipeline* get_active_pipeline();
	
protected:
	oclraster(const char* callpath_, const char* datapath_) = delete;
	~oclraster() = delete;
	oclraster& operator=(const oclraster&) = delete;
	
	static pipeline* active_pipeline;
	
	// window event handlers
	static event::handler* event_handler_fnctr;
	static bool event_handler(EVENT_TYPE type, shared_ptr<event_object> obj);

};

#endif
