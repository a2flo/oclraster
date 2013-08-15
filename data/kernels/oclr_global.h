
#ifndef __OCLRASTER_GLOBAL_H__
#define __OCLRASTER_GLOBAL_H__

// workaround for the nvidia compiler ...
#if defined(UNDEF__APPLE__)
#undef __APPLE__
#endif

#if (!defined(CPU) && !defined(OCLRASTER_CUDA_CL))
#if !(defined(OS_X_VERSION) && (OS_X_VERSION >= 1090))
#undef printf
#define printf(x, ...)
#endif
#endif

#if !defined(OCLRASTER_CUDA_CL)
#define OCLRASTER_FUNC inline
#endif

#if defined(__clang__)
#define FUNC_OVERLOAD __attribute__((overloadable))
#else
#define FUNC_OVERLOAD
#endif

// ignore all "no previous prototype" warnings
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

#if defined(PLATFORM_NVIDIA)
#pragma OPENCL EXTENSION cl_nv_compiler_options : enable
#endif

#if defined(PLATFORM_APPLE)

// the required gl_sharing extension has changed to cl_khr_gl_sharing in 10.8.3,
// even though it's not explicitly supported/exposed by the driver/runtime
#if (OS_X_VERSION < 1083)
#pragma OPENCL EXTENSION cl_APPLE_gl_sharing : enable
#else
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable
#endif

#else

// note: amd devices support this, but don't expose the extension and won't compile if this is enabled
#if !defined(PLATFORM_AMD)
#pragma OPENCL EXTENSION cl_khr_gl_sharing : enable
#endif

#endif

// these are part of OpenCL 1.2 core now
#if (!defined(CL_VERSION_1_2) || \
	 __OPENCL_C_VERSION__ < CL_VERSION_1_2)
#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
#if defined(OCLRASTER_DOUBLE_SUPPORT)
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif
#endif

//
#if !defined(cl_khr_fp16)
typedef struct __attribute__((aligned(sizeof(half)))) { ushort _; } oclr_half;
typedef struct __attribute__((aligned(sizeof(half) * 2))) { ushort _; } oclr_half2;
typedef struct __attribute__((aligned(sizeof(half) * 4))) { ushort _; } oclr_half3;
typedef struct __attribute__((aligned(sizeof(half) * 4))) { ushort _; } oclr_half4;
#else
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
typedef half oclr_half;
typedef half2 oclr_half2;
typedef half3 oclr_half3;
typedef half4 oclr_half4;
#endif

//
#define print_float3(vec) { \
	printf(#vec": (%.10f %.10f %.10f)\n", vec.x, vec.y, vec.z); \
}
#define print_float4(vec) { \
	printf(#vec": (%.10f %.10f %.10f %.10f)\n", vec.x, vec.y, vec.z, vec.w); \
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
#define oclraster_struct typedef struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))
#define oclraster_in typedef struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))
#define oclraster_out typedef struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))
#define oclraster_uniforms typedef struct __attribute__((packed, aligned(OCLRASTER_STRUCT_ALIGNMENT)))

// unsigned int on host side
enum PRIMITIVE_TYPE {
	PT_TRIANGLE,
	PT_TRIANGLE_STRIP,
	PT_TRIANGLE_FAN
};

// batch: 2 header bytes (#passing triangles), n bytes passing primitive mask (8 primitives per byte)
#define BATCH_HEADER_SIZE (1u)
#define BATCH_BYTE_COUNT (BATCH_SIZE / 8u)
#define BATCH_PRIMITIVE_COUNT (BATCH_SIZE - BATCH_HEADER_SIZE)

#endif
