/*  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "oclraster/oclraster.h"
#include "oclraster/oclraster_version.h"
#include "cl/opencl.hpp"
#include "core/gl_support.hpp"
#include "pipeline/framebuffer.h"
#include "pipeline/pipeline.h"

#if defined(__APPLE__)
#if !defined(OCLRASTER_IOS)
#include "osx/osx_helper.hpp"
#else
#include "ios/ios_helper.h"
#endif
#endif

// init statics
pipeline* oclraster::active_pipeline = nullptr;

#if defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
// from transform_program.cpp and rasterization_program.cpp for debugging purposes:
extern string template_transform_program;
extern string template_rasterization_program;
#endif

// dll main for windows dll export
#if defined(__WINDOWS__)
BOOL APIENTRY DllMain(HANDLE hModule floor_unused, DWORD ul_reason_for_call, LPVOID lpReserved floor_unused);
BOOL APIENTRY DllMain(HANDLE hModule floor_unused, DWORD ul_reason_for_call, LPVOID lpReserved floor_unused) {
	switch(ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}
#endif // __WINDOWS__

/*! this is used to set an absolute data path depending on call path (path from where the binary is called/started),
 *! which is mostly needed when the binary is opened via finder under os x or any file manager under linux
 */
void oclraster::init(const char* callpath_, const char* datapath_) {
	floor::init(callpath_, datapath_);
	// TODO: window title?
	// TODO: ocl->add_internal_kernels(...);
}

void oclraster::destroy() {
	log_debug("destroying oclraster ...");
	floor::destroy();
}

void oclraster::set_active_pipeline(pipeline* active_pipeline_) {
	active_pipeline = active_pipeline_;
}

pipeline* oclraster::get_active_pipeline() {
	return active_pipeline;
}
