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

// batch size (used in the binner and rasterization stage) - for now this is fixed at 256
// amount of primitives per batch: 256 (bits in total) - 2 (header bytes) * 8 = 240
#define OCLRASTER_BATCH_SIZE (256u)
// if you need to change these, also change them in data/kernels/oclr_global.h
#define OCLRASTER_BATCH_HEADER_SIZE (2u)
#define OCLRASTER_BATCH_BYTE_COUNT (OCLRASTER_BATCH_SIZE / 8u)
#define OCLRASTER_BATCH_PRIMITIVE_COUNT (OCLRASTER_BATCH_SIZE - OCLRASTER_BATCH_HEADER_SIZE * 8u)
#define OCLRASTER_BATCH_PRIMITIVE_BYTE_COUNT (OCLRASTER_BATCH_PRIMITIVE_COUNT / 8u)

// uses kernel templates from the data/kernels/ folder instead of the internal ones
#if !defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
#define OCLRASTER_INTERNAL_PROGRAM_DEBUG (1)
#endif

// this defines the minimum alignment for all oclraster structs (should at least be 16)
#define OCLRASTER_STRUCT_ALIGNMENT (16)
#define oclraster_struct struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))

// if this is enabled, the pipeline will do a FXAA pass in the swap function
#if !defined(OCLRASTER_FXAA)
//#define OCLRASTER_FXAA (1)
#endif

#endif
