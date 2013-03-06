
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
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

//
#if !defined(cl_khr_fp16)

#if defined(__clang__)
typedef half oclr_half;
typedef __attribute__(( ext_vector_type(2) ))  half oclr_half2;
typedef __attribute__(( ext_vector_type(3) ))  half oclr_half3;
typedef __attribute__(( ext_vector_type(4) ))  half oclr_half4;
typedef __attribute__(( ext_vector_type(8) ))  half oclr_half8;
typedef __attribute__(( ext_vector_type(16) )) half oclr_half16;
#else
// TODO: define correct half types on platforms without fp16/half support
typedef half oclr_half;
typedef half2 oclr_half2;
typedef half3 oclr_half3;
typedef half4 oclr_half4;
typedef half8 oclr_half8;
typedef half16 oclr_half16;
#endif

#if !defined(PLATFORM_APPLE) || \
	(defined(PLATFORM_APPLE) && defined(CPU))
float2 FUNC_OVERLOAD convert_float2(oclr_half2 vec) {
	return (float2)(convert_float(vec.x), convert_float(vec.y));
}
float3 FUNC_OVERLOAD convert_float3(oclr_half3 vec) {
	return (float3)(convert_float(vec.x), convert_float(vec.y), convert_float(vec.z));
}
float4 FUNC_OVERLOAD convert_float4(oclr_half4 vec) {
	return (float4)(convert_float(vec.x), convert_float(vec.y), convert_float(vec.z), convert_float(vec.w));
}
#endif

#endif

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

#endif
