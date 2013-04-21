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

#ifndef __OCLRASTER_SUPPORT_TEXMAN_H__
#define __OCLRASTER_SUPPORT_TEXMAN_H__

#include <oclraster_support/global.h>

class pipeline;
class image;
class OCLRASTER_API texture_manager {
public:
	texture_manager() = delete;
	~texture_manager() = delete;
	
	static void init(pipeline* p);
	static void destroy();
	
	static image* get_dummy_texture();
	
	static image* add_texture(const string& filename);
	
protected:
	static pipeline* oclr_pipeline;
	static image* dummy_texture;
	static unordered_map<string, image*> images;
	
};

#endif
