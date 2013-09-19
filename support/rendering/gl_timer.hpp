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

#ifndef __OCLRASTER_SUPPORT_GL_TIMER_HPP__
#define __OCLRASTER_SUPPORT_GL_TIMER_HPP__

#include <oclraster_support/global.hpp>

// TODO: implement this (for now this is just a dummy class)
class FLOOR_API gl_timer {
public:
	//
	gl_timer() = delete;
	~gl_timer() = delete;
	
	static constexpr size_t stored_frames = 16;
	struct frame_info {
		struct query_object {
			const string identifier;
			unsigned int query_ref;
			float3 color;
			unsigned long long int time;
		};
		vector<query_object> queries;
		bool done = false;
		bool available = false;
	};
	static const frame_info* get_last_available_frame() { return nullptr; }
	
	static void init() {}
	static void destroy() {}
	static void state_check() {}
	
	static void start_frame() {}
	static void mark(const string& identifier floor_unused) {}
	static void stop_frame() {}
	
};

#endif
