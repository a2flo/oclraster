
#ifndef __OCLRASTER_GLOBAL_H__
#define __OCLRASTER_GLOBAL_H__

// workaround for the nvidia compiler ...
#if defined(UNDEF__APPLE__)
#undef __APPLE__
#endif

#if (!defined(CPU) && !defined(OCLRASTER_CUDA_CL)) || false
#undef printf
#define printf(x, ...)
#endif

#if !defined(OCLRASTER_CUDA_CL)
#define OCLRASTER_FUNC inline
#endif

#if defined(PLATFORM_NVIDIA)
#pragma OPENCL EXTENSION cl_nv_compiler_options : enable
#endif

#if defined(PLATFORM_APPLE)
// TODO: in recent os x version this has be changed to cl_khr_gl_sharing, even though it's not
// explicitly supported/exposed by the driver/runtime
//#pragma OPENCL EXTENSION cl_APPLE_gl_sharing : enable
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable
#else
// note: amd devices support this, but don't expose the extension and won't compile if this is enabled
#if !defined(PLATFORM_AMD)
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable
#endif
#endif

#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

//
#define print_float3(vec) { \
	printf(#vec": (%f %f %f)\n", vec.x, vec.y, vec.z); \
}
#define print_float4(vec) { \
	printf(#vec": (%f %f %f %f)\n", vec.x, vec.y, vec.z, vec.w); \
}
#define print_mat4(mat) { \
	printf(#mat":\n/ %f %f %f %f \\\n| %f %f %f %f |\n| %f %f %f %f |\n\\ %f %f %f %f /\n", \
		   mat.m[0].x, mat.m[0].y, mat.m[0].z, mat.m[0].w, \
		   mat.m[1].x, mat.m[1].y, mat.m[1].z, mat.m[1].w, \
		   mat.m[2].x, mat.m[2].y, mat.m[2].z, mat.m[2].w, \
		   mat.m[3].x, mat.m[3].y, mat.m[3].z, mat.m[3].w); \
}

// well, this is awkward
#if !defined(NULL)
#define NULL 0
#endif
#if !defined(nullptr)
#define nullptr 0
#endif

// TODO: create an extra header for this
#define oclraster_in typedef struct __attribute__((packed, aligned(16)))
#define oclraster_out typedef struct __attribute__((packed, aligned(16)))
#define oclraster_uniforms typedef struct __attribute__((packed, aligned(16)))

// ignore all "no previous prototype" warnings
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#endif
