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

#include "oclraster/oclraster.hpp"
#include "oclraster/oclraster_version.hpp"
#include "cl/opencl.hpp"
#include "core/gl_support.hpp"
#include "pipeline/framebuffer.hpp"
#include "pipeline/pipeline.hpp"

#if defined(__APPLE__)
#if !defined(OCLRASTER_IOS)
#include "osx/osx_helper.hpp"
#else
#include "ios/ios_helper.hpp"
#endif
#endif

// init statics
pipeline* oclraster::active_pipeline = nullptr;
event::handler* oclraster::event_handler_fnctr = nullptr;

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
	floor::set_caption("oclraster");
	
	// print out oclraster info
	log_debug("%s", (OCLRASTER_VERSION_STRING).c_str());
	
	// create image type/channel mappings
	static const array<vector<cl_channel_type>, (size_t)IMAGE_TYPE::__MAX_TYPE> type_mapping {
		{
			{}, // NONE
			{ CL_SNORM_INT8, CL_SIGNED_INT8 }, // INT_8
			{ CL_SNORM_INT16, CL_SIGNED_INT16 }, // INT_16
			{ CL_SIGNED_INT32 }, // INT_32
			{  }, // INT_64 (not supported by opencl)
			{ CL_UNORM_INT8, CL_UNSIGNED_INT8 }, // UINT_8
			{ CL_UNORM_INT16, CL_UNSIGNED_INT16 }, // UINT_16
			{ CL_UNSIGNED_INT32 }, // UINT_32
			{  }, // UINT_64 (not supported by opencl)
			{ CL_HALF_FLOAT }, // FLOAT_16
			{ CL_FLOAT }, // FLOAT_32
			{  } // FLOAT_64 (not supported by opencl)
		}
	};
	static const array<vector<cl_channel_order>, (size_t)IMAGE_CHANNEL::__MAX_CHANNEL> channel_mapping {
		{
			{}, // NONE
			{ CL_R, CL_Rx, CL_INTENSITY, CL_LUMINANCE }, // R
			{ CL_RG, CL_RGx }, // RG
			{ CL_RGB, CL_RGBx }, // RGB
			{ CL_RGBA, CL_BGRA, CL_ARGB } // RGBA
		}
	};
	
	// fill internal_image_format_mapping with appropriate info/mappings
	for(size_t data_idx = 0; data_idx < type_mapping.size(); data_idx++) {
		if(type_mapping[data_idx].empty()) continue;
		for(size_t channel_idx = 0; channel_idx < channel_mapping.size(); channel_idx++) {
			if(channel_mapping[channel_idx].empty()) continue;
			bool found = false;
			for(const auto& req_data_type : type_mapping[data_idx]) {
				for(const auto& req_channel_type : channel_mapping[channel_idx]) {
					const auto& img_formats = ocl->get_image_formats();
					for(const auto& format : img_formats) {
						if(req_data_type == format.image_channel_data_type &&
						   req_channel_type == format.image_channel_order) {
							image::internal_image_format_mapping[data_idx][channel_idx] = cl::ImageFormat(req_channel_type, req_data_type);
							found = true;
#if 0
							oclr_log("native image support: %s -> %s %s",
									 image_type { (IMAGE_TYPE)data_idx, (IMAGE_CHANNEL)channel_idx }.to_string(),
									 channel_type_to_string(format.image_channel_data_type),
									 channel_order_to_string(format.image_channel_order));
#endif
							break;
						}
					}
					if(found) break;
				}
				if(found) break;
			}
		}
	}
	
#if defined(__APPLE__)
	if(ocl->is_gpu_support()) {
		// workaround: when a shared cpu/gpu context is used, apple falsely advertises CL_Rx, CL_RGx, CL_RGBx in combination with CL_FLOAT
		// as supported, when they're actually not -> use CL_R, CL_RG, CL_RGB instead, which do work, but are not officially listed
		auto& r_float_mapping = image::internal_image_format_mapping[(size_t)IMAGE_TYPE::FLOAT_32][(size_t)IMAGE_CHANNEL::R];
		auto& rg_float_mapping = image::internal_image_format_mapping[(size_t)IMAGE_TYPE::FLOAT_32][(size_t)IMAGE_CHANNEL::RG];
		auto& rgb_float_mapping = image::internal_image_format_mapping[(size_t)IMAGE_TYPE::FLOAT_32][(size_t)IMAGE_CHANNEL::RGB];
		if(r_float_mapping.image_channel_data_type != 0 && r_float_mapping.image_channel_order != 0) {
			r_float_mapping = cl::ImageFormat(CL_R, CL_FLOAT);
		}
		if(rg_float_mapping.image_channel_data_type != 0 && rg_float_mapping.image_channel_order != 0) {
			rg_float_mapping = cl::ImageFormat(CL_RG, CL_FLOAT);
		}
		if(rgb_float_mapping.image_channel_data_type != 0 && rgb_float_mapping.image_channel_order != 0) {
			rgb_float_mapping = cl::ImageFormat(CL_RGB, CL_FLOAT);
		}
	}
#endif
	
	// add global defines that have to be always present when compiling oclraster kernels
	ocl->add_global_kernel_defines(// just define this everywhere to make using image support
								   // easier without having to specify this every time
								   " -DOCLRASTER_IMAGE_HEADER_SIZE="+size_t2string(image::header_size())+
								   
								   // the same goes for the general struct alignment
								   // TODO: FLOOR_STRUCT_ALIGNMENT is already present, use it?
								   " -DOCLRASTER_STRUCT_ALIGNMENT="+uint2string(OCLRASTER_STRUCT_ALIGNMENT));
	
	// add "kernel reload" event handler (must be done before calling "add_internal_kernels", which will trigger a reload)
	event_handler_fnctr = new event::handler(&oclraster::event_handler);
	floor::get_event()->add_internal_event_handler(*event_handler_fnctr, EVENT_TYPE::KERNEL_RELOAD);
	
	// finally: add internal kernels
	ocl->add_internal_kernels(vector<opencl_base::internal_kernel_info> {
		{ "BIN_RASTERIZE", "bin_rasterize.cl", "oclraster_bin",
			" -DBIN_SIZE="+uint2string(OCLRASTER_BIN_SIZE)+
			" -DBATCH_SIZE="+uint2string(OCLRASTER_BATCH_SIZE)
		},
		
		{ "PROCESSING.PERSPECTIVE", "processing.cl", "oclraster_processing",
			" -DBIN_SIZE="+uint2string(OCLRASTER_BIN_SIZE)+
			" -DBATCH_SIZE="+uint2string(OCLRASTER_BATCH_SIZE)+
			" -DOCLRASTER_PROJECTION_PERSPECTIVE"
		},
		
		{ "PROCESSING.ORTHOGRAPHIC", "processing.cl", "oclraster_processing",
			" -DBIN_SIZE="+uint2string(OCLRASTER_BIN_SIZE)+
			" -DBATCH_SIZE="+uint2string(OCLRASTER_BATCH_SIZE)+
			" -DOCLRASTER_PROJECTION_ORTHOGRAPHIC"
		}
		
#if defined(OCLRASTER_FXAA)
		,
		{ "FXAA.LUMA", "luma_pass.cl", "framebuffer_luma", "" },
		{ "FXAA", "fxaa_pass.cl", "framebuffer_fxaa", "" }
#endif
	});
}

void oclraster::destroy() {
	log_debug("destroying oclraster ...");
	
	floor::acquire_context();
	floor::get_event()->remove_event_handler(*event_handler_fnctr);
	delete event_handler_fnctr;
	delete_clear_kernels();
	floor::release_context();
	
	floor::destroy();
}

void oclraster::start_draw() {
	floor::start_draw();
	
	// draws ogl stuff
#if !defined(OCLRASTER_USE_DRAW_PIXELS)
	glBindFramebuffer(GL_FRAMEBUFFER, FLOOR_DEFAULT_FRAMEBUFFER);
#endif
	glViewport(0, 0, floor::get_width(), floor::get_height());
	
	// clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void oclraster::stop_draw() {
	if(active_pipeline != nullptr) {
		active_pipeline->swap();
	}
	floor::stop_draw();
}

void oclraster::set_active_pipeline(pipeline* active_pipeline_) {
	active_pipeline = active_pipeline_;
}

pipeline* oclraster::get_active_pipeline() {
	return active_pipeline;
}

bool oclraster::event_handler(EVENT_TYPE type, shared_ptr<event_object> obj floor_unused) {
	if(type == EVENT_TYPE::KERNEL_RELOAD) {
#if defined(OCLRASTER_INTERNAL_PROGRAM_DEBUG)
		template_transform_program = file_io::file_to_string(floor::data_path("kernels/template_transform_program.cl"));
		if(template_transform_program == "") {
			log_error("failed to load template_transform_program!");
		}
		template_rasterization_program = file_io::file_to_string(floor::data_path("kernels/template_rasterization_program.cl"));
		if(template_rasterization_program == "") {
			log_error("failed to load template_rasterization_program!");
		}
#endif
		
		// deletes all framebuffer clear kernels
		delete_clear_kernels();
		
		return true;
	}
	return false;
}
