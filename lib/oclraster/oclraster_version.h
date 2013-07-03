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

#ifndef __OCLRASTER_VERSION__
#define __OCLRASTER_VERSION__

// oclraster version and build/compiler info
#include "build_version.h"
#include "core/util.h"
#define OCLRASTER_MAJOR_VERSION "0"
#define OCLRASTER_MINOR_VERSION "0"
#define OCLRASTER_REVISION_VERSION "4"
#define OCLRASTER_DEV_STAGE_VERSION "d2"
#define OCLRASTER_BUILD_TIME __TIME__
#define OCLRASTER_BUILD_DATE __DATE__

#if defined(OCLRASTER_DEBUG) || defined(DEBUG)
#define OCLRASTER_DEBUG_STR " (debug)"
#else
#define OCLRASTER_DEBUG_STR ""
#endif

#if defined(_MSC_VER)
#define OCLRASTER_COMPILER "VC++ "+size_t2string(_MSC_VER)
#elif (defined(__GNUC__) && !defined(__llvm__) && !defined(__clang__))
#define OCLRASTER_COMPILER string("GCC ")+__VERSION__
#elif (defined(__GNUC__) && defined(__llvm__) && !defined(__clang__))
#define OCLRASTER_COMPILER string("LLVM-GCC ")+__VERSION__
#elif defined(__clang__)
#define OCLRASTER_COMPILER string("Clang ")+__clang_version__
#else
#define OCLRASTER_COMPILER "unknown compiler"
#endif

#define OCLRASTER_LIBCXX_PREFIX " and "
#if defined(_LIBCPP_VERSION)
#define OCLRASTER_LIBCXX OCLRASTER_LIBCXX_PREFIX+"libc++ "+size_t2string(_LIBCPP_VERSION)
#elif defined(__GLIBCXX__)
#define OCLRASTER_LIBCXX OCLRASTER_LIBCXX_PREFIX+"libstdc++ "+size_t2string(__GLIBCXX__)
#else
#define OCLRASTER_LIBCXX ""
#endif

#if !defined(OCLRASTER_IOS)
#define OCLRASTER_PLATFORM (sizeof(void*) == 4 ? "x86" : (sizeof(void*) == 8 ? "x64" : "unknown"))
#else
#define OCLRASTER_PLATFORM (sizeof(void*) == 4 ? "ARM" : (sizeof(void*) == 8 ? "ARM64" : "unknown"))
#endif

#define OCLRASTER_VERSION_STRING (string("oclraster ")+OCLRASTER_PLATFORM+OCLRASTER_DEBUG_STR \
" v"+(OCLRASTER_MAJOR_VERSION)+"."+(OCLRASTER_MINOR_VERSION)+"."+(OCLRASTER_REVISION_VERSION)+(OCLRASTER_DEV_STAGE_VERSION)+"-"+size_t2string(OCLRASTER_BUILD_VERSION)+\
" ("+OCLRASTER_BUILD_DATE+" "+OCLRASTER_BUILD_TIME+") built with "+string(OCLRASTER_COMPILER+OCLRASTER_LIBCXX))

#define OCLRASTER_SOURCE_URL "http://www.albion2.org"


// compiler checks:
// msvc check
#if defined(_MSC_VER)
#if (_MSC_VER <= 1800)
#error "Sorry, but you need MSVC 13.0+ (VS 2014+) to compile oclraster"
#endif

// clang check
#elif defined(__clang__)
#if !defined(__clang_major__) || !defined(__clang_minor__) || (__clang_major__ < 3) || (__clang_major__ == 3 && __clang_minor__ < 2)
#error "Sorry, but you need Clang 3.2+ to compile oclraster"
#endif

// gcc check
#elif defined(__GNUC__)
#if (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 9)
#error "Sorry, but you need GCC 4.9+ to compile oclraster"
#endif

// just fall through ...
#else
#endif

#endif
