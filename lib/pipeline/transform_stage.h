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

#ifndef __OCLRASTER_TRANSFORM_STAGE_H__
#define __OCLRASTER_TRANSFORM_STAGE_H__

#include "cl/opencl.h"

struct draw_state;
class transform_stage {
public:
	transform_stage();
	~transform_stage();
	
	// per vertex / "attribute buffer"?
	struct vertex_buffer {
		// <name, size>
		// ex: { { "vertex", 12 }, { "normal", 12 }, ... }
		vector<pair<string, unsigned int>> elements { };
		opencl_base::buffer_object* buffer { nullptr };
	};
	
	struct index_buffer {
		unsigned int index_count { 0 };
		opencl_base::buffer_object* buffer { nullptr };
	};
	
	struct constant_buffer {
		// <name, ptr to data>
		// ex: { { "mvm", ... }, { "pm", ... }, { "camera_position", ... }, ... }
		vector<pair<string, void*>> constants;
	};
	
	struct transformed_buffer {
		vector<pair<string, unsigned int>> elements;
		opencl_base::buffer_object* buffer;
	};
	
	//
	void transform(draw_state& state,
				   const unsigned int& num_elements);

protected:
	opencl_base::buffer_object* const_buffer_tp = nullptr;

};

#endif
