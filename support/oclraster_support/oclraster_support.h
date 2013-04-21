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

#ifndef __OCLRASTER_SUPPORT_H__
#define __OCLRASTER_SUPPORT_H__

#include "oclraster_support/global.h"
#include <oclraster/oclraster.h>
#include <oclraster/pipeline/pipeline.h>

class gui;
class OCLRASTER_API oclraster_support {
public:
	static void init(pipeline* oclr_pipeline);
	static void destroy();
	
	static pipeline* get_pipeline();
	static gui* get_gui();

protected:
	oclraster_support() = delete;
	~oclraster_support() = delete;
	oclraster_support& operator=(const oclraster_support&) = delete;
	
	static pipeline* oclr_pipeline;
	static gui* ui;
	
};

#endif
