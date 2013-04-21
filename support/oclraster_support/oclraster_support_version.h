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

#ifndef __OCLRASTER_SUPPORT_SUPPORT_VERSION__
#define __OCLRASTER_SUPPORT_VERSION__

//
#include "build_version.h"
#include <oclraster/oclraster_version.h>

#define OCLRASTER_SUPPORT_VERSION_STRING (string("oclraster_support ")+OCLRASTER_PLATFORM+OCLRASTER_DEBUG_STR \
" v"+(OCLRASTER_MAJOR_VERSION)+"."+(OCLRASTER_MINOR_VERSION)+"."+(OCLRASTER_REVISION_VERSION)+(OCLRASTER_DEV_STAGE_VERSION)+"-"+size_t2string(OCLRASTER_SUPPORT_BUILD_VERSION)+\
" ("+OCLRASTER_BUILD_DATE+" "+OCLRASTER_BUILD_TIME+") built with "+string(OCLRASTER_COMPILER+OCLRASTER_LIBCXX))

#endif
