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

#ifndef __OCLRASTER_OPENCL_H__
#define __OCLRASTER_OPENCL_H__

#include "oclraster/global.h"
#include "core/file_io.h"
#include "core/core.h"
#include "core/vector2.h"
#include "core/gl_support.h"

// necessary for now (when compiling with opencl 1.2+ headers)
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS 1

#if defined(__APPLE__)
#include <OpenCL/OpenCL.h>
#include <OpenCL/cl.h>
#include <OpenCL/cl_platform.h>
#include <OpenCL/cl_ext.h>
#include <OpenCL/cl_gl.h>
#if !defined(OCLRASTER_IOS)
#include <OpenGL/CGLContext.h>
#include <OpenGL/CGLCurrent.h>
#include <OpenGL/CGLDevice.h>
#endif
#else
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <CL/cl_ext.h>
#include <CL/cl_gl.h>
#endif

#define __CL_ENABLE_EXCEPTIONS
#include "cl/cl.hpp"

#define CLINFO_STR_SIZE 65536*2

//
#if defined(OCLRASTER_CUDA_CL)
#if defined(__APPLE__)
#include <CUDA/cuda.h>
#include <CUDA/cudaGL.h>
#else
#include <cuda.h>
#include <cudaGL.h>
#endif
#endif

/*! @class opencl_base
 *  @brief opencl_base interface
 */

class OCLRASTER_API opencl_base {
public:
	struct kernel_object;
	struct buffer_object;
	struct device_object;
	
	opencl_base& operator=(const opencl_base&) = delete;
	opencl_base(const opencl_base&) = delete;
	opencl_base();
	virtual ~opencl_base();
	
	bool is_supported() { return supported; }
	bool is_cpu_support();
	bool is_gpu_support();
	
	enum class DEVICE_TYPE : unsigned int {
		NONE,
		FASTEST_GPU,
		FASTEST_CPU,
		ALL_GPU,
		ALL_CPU,
		ALL_DEVICES,
		GPU0,
		GPU1,
		GPU2,
		GPU4,
		GPU5,
		GPU6,
		GPU7,
		GPU255 = GPU0+254,
		CPU0,
		CPU1,
		CPU2,
		CPU3,
		CPU4,
		CPU5,
		CPU6,
		CPU7,
		CPU255 = CPU0+254
	};
	device_object* get_device(const DEVICE_TYPE& device);
	device_object* get_active_device();
	const vector<device_object*>& get_devices() const;
	
	enum class PLATFORM_VENDOR {
		NVIDIA,
		INTEL,
		AMD,
		APPLE,
		UNKNOWN
	};
	// <vendor, index/identifier for use in oclraster config>
	static vector<pair<PLATFORM_VENDOR, string>> get_platforms();
	static string platform_vendor_to_str(const PLATFORM_VENDOR& pvendor);
	PLATFORM_VENDOR get_platform_vendor() const;
	
	enum class VENDOR {
		NVIDIA,
		INTEL,
		ATI,
		AMD,
		APPLE,
		UNKNOWN
	};
	
	//! buffer flags (associated kernel => buffer has been set as a kernel argument, at least once and the latest one for an index)
	enum class BUFFER_FLAG : unsigned int {
		NONE				= (0),
		READ				= (1 << 0),			//!< enum read only buffer (kernel POV)
		WRITE				= (1 << 1),			//!< enum write only buffer (kernel POV)
		READ_WRITE			= (READ | WRITE),	//!< enum read and write buffer (kernel POV)
		INITIAL_COPY		= (1 << 2),			//!< enum the specified data will be copied to the buffer at creation time
		COPY_ON_USE			= (1 << 3),			//!< enum the specified data will be copied to the buffer each time an associated kernel is being used (that is right before kernel execution)
		USE_HOST_MEMORY		= (1 << 4),			//!< enum buffer memory will be allocated in host memory
		READ_BACK_RESULT	= (1 << 5),			//!< enum every time an associated kernel has been executed, the result buffer data will be read back/copied to the specified pointer location
		DELETE_AFTER_USE	= (1 << 6),			//!< enum the buffer will be deleted after its first use (after an associated kernel has been executed)
		BLOCK_ON_READ		= (1 << 7),			//!< enum the read command is blocking, all data will be read/copied before program continuation
		BLOCK_ON_WRITE		= (1 << 8),			//!< enum the write command is blocking, all data will be written before program continuation
		OPENGL_BUFFER		= (1 << 9),			//!< enum determines if a buffer is a shared opengl buffer/image/memory object
	};
	enum_class_bitwise_or(BUFFER_FLAG)
	enum_class_bitwise_and(BUFFER_FLAG)

	virtual void init(bool use_platform_devices = false, const size_t platform_index = 0,
					  const set<string> device_restriction = set<string> {},
					  const bool gl_sharing = true) = 0;
	void reload_kernels();
	
	void use_kernel(const string& identifier);
	void run_kernel();
	void run_kernel(const string& identifier);
	virtual void run_kernel(kernel_object* kernel_obj) = 0;
	kernel_object* get_cur_kernel() { return cur_kernel; }
	virtual void finish() = 0;
	virtual void flush() = 0;
	virtual void barrier() = 0;
	virtual void activate_context() = 0;
	virtual void deactivate_context() = 0;
	
	kernel_object* add_kernel_file(const string& identifier, const string& file_name, const string& func_name, const string additional_options = "");
	virtual kernel_object* add_kernel_src(const string& identifier, const string& src, const string& func_name, const string additional_options = "") = 0;
	void delete_kernel(const string& identifier);
	virtual void delete_kernel(kernel_object* kernel_obj) = 0;
	
	virtual buffer_object* create_buffer(BUFFER_FLAG type, size_t size, void* data = nullptr) = 0;
	virtual buffer_object* create_image2d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, void* data = nullptr) = 0;
	virtual buffer_object* create_image3d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, size_t depth, void* data = nullptr) = 0;
	virtual buffer_object* create_ogl_buffer(BUFFER_FLAG type, GLuint ogl_buffer) = 0;
	virtual buffer_object* create_ogl_image2d_buffer(BUFFER_FLAG type, GLuint texture, GLenum target = GL_TEXTURE_2D) = 0;
	virtual buffer_object* create_ogl_image2d_renderbuffer(BUFFER_FLAG type, GLuint renderbuffer) = 0;
	virtual void delete_buffer(buffer_object* buffer_obj) = 0;
	virtual void write_buffer(buffer_object* buffer_obj, const void* src, const size_t offset = 0, const size_t size = 0) = 0;
	virtual void write_image2d(buffer_object* buffer_obj, const void* src, size2 origin, size2 region) = 0;
	virtual void write_image3d(buffer_object* buffer_obj, const void* src, size3 origin, size3 region) = 0;
	virtual void read_buffer(void* dst, buffer_object* buffer_obj) = 0;
	virtual void* map_buffer(buffer_object* buffer_obj, BUFFER_FLAG access_type, bool blocking = true) = 0;
	virtual void unmap_buffer(buffer_object* buffer_obj, void* map_ptr) = 0;
	void set_manual_gl_sharing(buffer_object* gl_buffer_obj, const bool state);
	
	virtual void set_active_device(const DEVICE_TYPE& dev) = 0;
	virtual bool set_kernel_argument(const unsigned int& index, buffer_object* arg) = 0;
	virtual bool set_kernel_argument(const unsigned int& index, const buffer_object* arg) = 0;
	virtual bool set_kernel_argument(const unsigned int& index, size_t size, void* arg) = 0;
	template<typename T> bool set_kernel_argument(const unsigned int& index, T&& arg);
	void set_kernel_range(const pair<cl::NDRange, cl::NDRange> range);
	virtual size_t get_kernel_work_group_size() const = 0;
	
	pair<cl::NDRange, cl::NDRange> compute_kernel_ranges(const size_t& work_items) const;
	pair<cl::NDRange, cl::NDRange> compute_kernel_ranges(const size_t& work_items_x, const size_t& work_items_y) const;
	pair<cl::NDRange, cl::NDRange> compute_kernel_ranges(const size_t& work_items_x, const size_t& work_items_y, const size_t& work_items_z) const;
	
	//! this is for manual handling only
	virtual void acquire_gl_object(buffer_object* gl_buffer_obj) = 0;
	virtual void release_gl_object(buffer_object* gl_buffer_obj) = 0;
	
	struct kernel_object {
		cl::Kernel* kernel = nullptr;
		cl::Program* program = nullptr;
		cl::NDRange global { 1 };
		cl::NDRange local { 1 };
		unsigned int arg_count = 0;
		bool has_ogl_buffers = false;
		vector<bool> args_passed;
		unordered_map<unsigned int, buffer_object*> buffer_args;
		string name = "";
		unordered_map<cl::CommandQueue*, cl::KernelFunctor> functors;
		
		kernel_object() : args_passed(), buffer_args() {}
		~kernel_object() {
			for(const auto& ba : buffer_args) {
				ba.second->associated_kernels.erase(this);
			}
			if(program != nullptr) delete program;
			if(kernel != nullptr) delete kernel;
		}
	};
	
	struct buffer_object {
		cl::Buffer* buffer = nullptr;
		cl::Image* image_buffer = nullptr;
		GLuint ogl_buffer = 0;
		bool manual_gl_sharing = false;
		void* data = nullptr;
		size_t size = 0;
		BUFFER_FLAG type = BUFFER_FLAG::NONE;
		cl_mem_flags flags = 0;
		cl::ImageFormat format = cl::ImageFormat(0, 0);
		size3 origin = size3(size_t(0));
		size3 region = size3(size_t(0));
		unordered_map<kernel_object*, vector<unsigned int>> associated_kernels; // kernels + argument numbers
		
		buffer_object() : associated_kernels() {}
		~buffer_object() {}
	};
	
	struct device_object {
		const cl::Device* device = nullptr;
		opencl_base::DEVICE_TYPE type = DEVICE_TYPE::NONE;
		opencl_base::VENDOR vendor_type = VENDOR::UNKNOWN;
		unsigned int units = 0;
		unsigned int clock = 0;
		cl_ulong mem_size = 0;
		cl_device_type internal_type = 0;
		string name = "";
		string vendor = "";
		string version = "";
		string driver_version = "";
		string extensions = "";
		
		cl_ulong max_alloc = 0;
		size_t max_wg_size = 0;
		size3 max_wi_sizes { 1, 1, 1 };
		size2 max_img_2d { 0, 0 };
		size3 max_img_3d { 0, 0, 0 };
		bool img_support = false;
		
		device_object() {}
		~device_object() {}
	};

	inline const string make_kernel_path(const string& file_name) const {
		return kernel_path_str + file_name;
	}
	
protected:
	SDL_Window* sdl_wnd;
	bool supported = true;
	
	string build_options;
	string nv_build_options;
	string kernel_path_str;
	
	virtual buffer_object* create_buffer_object(BUFFER_FLAG type, void* data = nullptr) = 0;
	void load_internal_kernels();
	void destroy_kernels();
	void check_compilation(const bool ret, const string& filename);
	virtual void log_program_binary(const kernel_object* kernel) = 0;
	
	bool has_vendor_device(VENDOR vendor_type);
	
	virtual const char* error_code_to_string(cl_int error_code) const = 0;
	
	cl::Context* context;
	cl::Platform* platform;
	PLATFORM_VENDOR platform_vendor = PLATFORM_VENDOR::UNKNOWN;
	vector<cl::Platform> platforms;
	vector<cl::Device> internal_devices;
	vector<device_object*> devices;
	device_object* active_device;
	device_object* fastest_cpu;
	device_object* fastest_gpu;
	vector<cl::ImageFormat> ro_formats;
	vector<cl::ImageFormat> wo_formats;
	vector<cl::ImageFormat> rw_formats;
	cl_int ierr;
	bool successful_internal_compilation;
	
	vector<buffer_object*> buffers;
	unordered_map<string, kernel_object*> kernels;
	kernel_object* cur_kernel;
	
	unordered_map<const cl::Device*, cl::CommandQueue*> queues;
	
	// identifier -> <file_name, func_name, options>
	unordered_map<string, tuple<string, string, string>> external_kernels;
	vector<tuple<string, string, string, string>> internal_kernels;
	
};

template<typename T> bool opencl_base::set_kernel_argument(const unsigned int& index, T&& arg) {
	try {
		set_kernel_argument(index, sizeof(T), (void*)&arg);
	}
	catch(cl::Error err) {
		oclr_error("%s (%d: %s)!", err.what(), err.err(), error_code_to_string(err.err()));
		return false;
	}
	catch(...) {
		oclr_error("unknown error!");
		return false;
	}
	
	// remove "references" of the last used buffer for this kernel and argument index (if there is one)
	if(cur_kernel->buffer_args.count(index) > 0) {
		auto& associated_kernels = cur_kernel->buffer_args[index]->associated_kernels[cur_kernel];
		associated_kernels.erase(remove(begin(associated_kernels), end(associated_kernels), index), end(associated_kernels));
		cur_kernel->buffer_args.erase(index);
	}
	return true;
}
	
/*! @class opencl
 *  @brief opencl interface
 */
class OCLRASTER_API opencl : public opencl_base {
public:
	//
	opencl(const char* kernel_path_, SDL_Window* wnd_, const bool clear_cache_);
	virtual ~opencl();
	
	virtual void init(bool use_platform_devices = false, const size_t platform_index = 0,
					  const set<string> device_restriction = set<string> {},
					  const bool gl_sharing = true);
	
	virtual void run_kernel(kernel_object* kernel_obj);
	
	virtual void finish();
	virtual void flush();
	virtual void barrier();
	virtual void activate_context();
	virtual void deactivate_context();
	
	virtual kernel_object* add_kernel_src(const string& identifier, const string& src, const string& func_name, const string additional_options = "");
	virtual void delete_kernel(kernel_object* kernel_obj);
	
	virtual buffer_object* create_buffer(BUFFER_FLAG type, size_t size, void* data = nullptr);
	virtual buffer_object* create_image2d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, void* data = nullptr);
	virtual buffer_object* create_image3d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, size_t depth, void* data = nullptr);
	virtual buffer_object* create_ogl_buffer(BUFFER_FLAG type, GLuint ogl_buffer);
	virtual buffer_object* create_ogl_image2d_buffer(BUFFER_FLAG type, GLuint texture, GLenum target = GL_TEXTURE_2D);
	virtual buffer_object* create_ogl_image2d_renderbuffer(BUFFER_FLAG type, GLuint renderbuffer);
	virtual void delete_buffer(buffer_object* buffer_obj);
	virtual void write_buffer(buffer_object* buffer_obj, const void* src, const size_t offset = 0, const size_t size = 0);
	virtual void write_image2d(buffer_object* buffer_obj, const void* src, size2 origin, size2 region);
	virtual void write_image3d(buffer_object* buffer_obj, const void* src, size3 origin, size3 region);
	virtual void read_buffer(void* dst, buffer_object* buffer_obj);
	virtual void* map_buffer(buffer_object* buffer_obj, BUFFER_FLAG access_type, bool blocking = true);
	virtual void unmap_buffer(buffer_object* buffer_obj, void* map_ptr);
	
	virtual void set_active_device(const DEVICE_TYPE& dev);
	virtual bool set_kernel_argument(const unsigned int& index, buffer_object* arg);
	virtual bool set_kernel_argument(const unsigned int& index, const buffer_object* arg);
	virtual bool set_kernel_argument(const unsigned int& index, size_t size, void* arg);
	virtual size_t get_kernel_work_group_size() const;
	
	virtual void acquire_gl_object(buffer_object* gl_buffer_obj);
	virtual void release_gl_object(buffer_object* gl_buffer_obj);
	
protected:
	virtual buffer_object* create_buffer_object(BUFFER_FLAG type, void* data = nullptr);
	virtual void log_program_binary(const kernel_object* kernel);
	virtual const char* error_code_to_string(cl_int error_code) const;
	
};

#if defined(OCLRASTER_CUDA_CL)
/*! @class cudacl
 *  @brief cudacl interface
 */
struct cuda_kernel_object;
class OCLRASTER_API cudacl : public opencl_base {
public:
	//
	cudacl(const char* kernel_path_, SDL_Window* wnd_, const bool clear_cache_);
	virtual ~cudacl();
	
	virtual void init(bool use_platform_devices = false, const size_t platform_index = 0,
					  const set<string> device_restriction = set<string> {},
					  const bool gl_sharing = true);
	
	virtual void run_kernel(kernel_object* kernel_obj);
	
	virtual void finish();
	virtual void flush();
	virtual void barrier();
	virtual void activate_context();
	virtual void deactivate_context();
	
	virtual kernel_object* add_kernel_src(const string& identifier, const string& src, const string& func_name, const string additional_options = "");
	virtual void delete_kernel(kernel_object* kernel_obj);
	
	virtual buffer_object* create_buffer(BUFFER_FLAG type, size_t size, void* data = nullptr);
	virtual buffer_object* create_image2d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, void* data = nullptr);
	virtual buffer_object* create_image3d_buffer(BUFFER_FLAG type, cl_channel_order channel_order, cl_channel_type channel_type, size_t width, size_t height, size_t depth, void* data = nullptr);
	virtual buffer_object* create_ogl_buffer(BUFFER_FLAG type, GLuint ogl_buffer);
	virtual buffer_object* create_ogl_image2d_buffer(BUFFER_FLAG type, GLuint texture, GLenum target = GL_TEXTURE_2D);
	virtual buffer_object* create_ogl_image2d_renderbuffer(BUFFER_FLAG type, GLuint renderbuffer);
	virtual void delete_buffer(buffer_object* buffer_obj);
	virtual void write_buffer(buffer_object* buffer_obj, const void* src, const size_t offset = 0, const size_t size = 0);
	virtual void write_image2d(buffer_object* buffer_obj, const void* src, size2 origin, size2 region);
	virtual void write_image3d(buffer_object* buffer_obj, const void* src, size3 origin, size3 region);
	virtual void read_buffer(void* dst, buffer_object* buffer_obj);
	virtual void* map_buffer(buffer_object* buffer_obj, BUFFER_FLAG access_type, bool blocking = true);
	virtual void unmap_buffer(buffer_object* buffer_obj, void* map_ptr);
	
	virtual void set_active_device(const DEVICE_TYPE& dev);
	virtual bool set_kernel_argument(const unsigned int& index, buffer_object* arg);
	virtual bool set_kernel_argument(const unsigned int& index, const buffer_object* arg);
	virtual bool set_kernel_argument(const unsigned int& index, size_t size, void* arg);
	virtual size_t get_kernel_work_group_size() const;
	
	virtual void acquire_gl_object(buffer_object* gl_buffer_obj);
	virtual void release_gl_object(buffer_object* gl_buffer_obj);
	
protected:
	bool valid = true;
	bool use_ptx_cache = false;
	string cache_path = "";
	string cc_target_str = "10";
	unsigned int cc_target = 0;
	vector<CUdevice*> cuda_devices;
	unordered_map<opencl_base::device_object*, const CUdevice*> device_map;
	unordered_map<const CUdevice*, CUcontext*> cuda_contexts;
	unordered_map<const CUdevice*, CUstream*> cuda_queues;
	unordered_map<opencl_base::buffer_object*, CUdeviceptr*> cuda_buffers;
	unordered_map<opencl_base::buffer_object*, CUgraphicsResource*> cuda_gl_buffers;
	unordered_map<CUgraphicsResource*, CUdeviceptr*> cuda_mapped_gl_buffers;
	unordered_map<opencl_base::kernel_object*, cuda_kernel_object*> cuda_kernels;
	
	//
	virtual buffer_object* create_buffer_object(BUFFER_FLAG type, void* data = nullptr);
	virtual void log_program_binary(const kernel_object* kernel);
	virtual const char* error_code_to_string(cl_int error_code) const;
	
};
#endif

#endif // __OPENCL_H__
