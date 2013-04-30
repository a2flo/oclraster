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

#ifndef __OCLRASTER_GLOBAL_H__
#define __OCLRASTER_GLOBAL_H__

#include "core/platform.h"

// bin x/y size in pixels
#define OCLRASTER_BIN_SIZE (32u)

// amount of primitives per batch (in the binner) - don't change this!
#define OCLRASTER_BATCH_SIZE (256u)

// uses kernel templates from the data/kernels/ folder instead of the internal one
#if !defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
//#define OCLRASTER_INTERNAL_PROGRAM_DEBUG (1)
#endif

// this defines the minimum alignment for all oclraster structs (should at least be 16)
#define OCLRASTER_STRUCT_ALIGNMENT (16)
#define oclraster_struct struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))

#endif
