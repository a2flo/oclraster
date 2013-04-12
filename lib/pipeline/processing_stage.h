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

#ifndef __OCLRASTER_PROCESSING_STAGE_H__
#define __OCLRASTER_PROCESSING_STAGE_H__

#include "cl/opencl.h"
#include "pipeline/stage_base.h"

enum class PRIMITIVE_TYPE : unsigned int;
struct draw_state;
class processing_stage : public stage_base {
public:
	processing_stage();
	virtual ~processing_stage();
	
	//
	void process(draw_state& state,
				 const PRIMITIVE_TYPE type,
				 const unsigned int& primitive_count);
	
protected:

};

#endif
