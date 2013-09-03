
#ifndef __OCLRASTER_IMAGE_H__
#define __OCLRASTER_IMAGE_H__

#include "oclr_global.h"
#include "oclr_math.h"

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

// note that the image (header) is actually 4096-byte aligned,
// but certain implementations (amd) can't handle aligned values > 128
typedef struct __attribute__((packed,
#if !defined(PLATFORM_AMD)
							  aligned(OCLRASTER_IMAGE_HEADER_SIZE)
#else
							  aligned(128)
#endif
							  )) {
	const ushort type;
	const ushort channel_order;
	const ushort width;
	const ushort height;
} image_header;
typedef global const image_header* image_header_ptr;

//
OCLRASTER_FUNC uint2 oclr_get_image_size(global const image_header* img) {
	return (uint2)(img->width, img->height);
}
OCLRASTER_FUNC unsigned int oclr_get_image_type(global const image_header* img) {
	return img->type;
}
OCLRASTER_FUNC unsigned int oclr_get_image_channel_order(global const image_header* img) {
	return img->channel_order;
}

//
OCLRASTER_FUNC float FUNC_OVERLOAD texel_mix(float x, float y, float a) { return linear_blend(x, y, a); }
OCLRASTER_FUNC float2 FUNC_OVERLOAD texel_mix(float2 x, float2 y, float a) { return linear_blend(x, y, a); }
OCLRASTER_FUNC float3 FUNC_OVERLOAD texel_mix(float3 x, float3 y, float a) { return linear_blend(x, y, a); }
OCLRASTER_FUNC float4 FUNC_OVERLOAD texel_mix(float4 x, float4 y, float a) { return linear_blend(x, y, a); }
#if defined(FLOOR_DOUBLE_SUPPORT)
OCLRASTER_FUNC double FUNC_OVERLOAD texel_mix(double x, double y, float a) { return linear_blend(x, y, (double)a); }
OCLRASTER_FUNC double2 FUNC_OVERLOAD texel_mix(double2 x, double2 y, float a) { return linear_blend(x, y, (double)a); }
OCLRASTER_FUNC double3 FUNC_OVERLOAD texel_mix(double3 x, double3 y, float a) { return linear_blend(x, y, (double)a); }
OCLRASTER_FUNC double4 FUNC_OVERLOAD texel_mix(double4 x, double4 y, float a) { return linear_blend(x, y, (double)a); }
#endif

#define INT_TEXEL_MIX(type, type_vec, ftype, one) \
OCLRASTER_FUNC type_vec FUNC_OVERLOAD texel_mix(type_vec x, type_vec y, float a) { \
	const ftype inv_a = one / (ftype)a; \
	const ftype inv_b = one / (one - (ftype)a); \
	return (x / (type)inv_a) + (y / (type)inv_b); \
}
INT_TEXEL_MIX(uint, uint, float, 1.0f)
INT_TEXEL_MIX(uint, uint2, float, 1.0f)
INT_TEXEL_MIX(uint, uint3, float, 1.0f)
INT_TEXEL_MIX(uint, uint4, float, 1.0f)
INT_TEXEL_MIX(int, int, float, 1.0f)
INT_TEXEL_MIX(int, int2, float, 1.0f)
INT_TEXEL_MIX(int, int3, float, 1.0f)
INT_TEXEL_MIX(int, int4, float, 1.0f)
#if defined(FLOOR_DOUBLE_SUPPORT)
INT_TEXEL_MIX(ulong, ulong, double, 1.0)
INT_TEXEL_MIX(ulong, ulong2, double, 1.0)
INT_TEXEL_MIX(ulong, ulong3, double, 1.0)
INT_TEXEL_MIX(ulong, ulong4, double, 1.0)
INT_TEXEL_MIX(long, long, double, 1.0)
INT_TEXEL_MIX(long, long2, double, 1.0)
INT_TEXEL_MIX(long, long3, double, 1.0)
INT_TEXEL_MIX(long, long4, double, 1.0)
#endif

// since certain opencl implementations don't allow bitwise (or any) operations on sampler_t,
// which are necessary for oclrasters buffer-based image support (aka software emulation),
// define a oclr_sampler_t which should easily convert to sampler_t (constructible by uint per spec)
// note: use this always and everywhere (except where emulating opencl functions)
typedef unsigned int oclr_sampler_t;

// pocl doesn't support all native image functions and the supported ones have a few issues
// also, pocls "native" image functions won't be faster than oclrasters software image functions
#if !defined(PLATFORM_POCL)

// image read functions for native images
OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_hw(read_only image2d_t img, const sampler_t sampler, const float2 coord) {
	return read_imagef(img, sampler, coord);
}
OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_hw(read_only image2d_t img, const sampler_t sampler, const uint2 coord) {
	return read_imagef(img, sampler, convert_int2(coord));
}
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_hw(read_only image2d_t img, const sampler_t sampler, const float2 coord) {
	return read_imagei(img, sampler, coord);
}
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_hw(read_only image2d_t img, const sampler_t sampler, const uint2 coord) {
	return read_imagei(img, sampler, convert_int2(coord));
}
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_hw(read_only image2d_t img, const sampler_t sampler, const float2 coord) {
	return read_imageui(img, sampler, coord);
}
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_hw(read_only image2d_t img, const sampler_t sampler, const uint2 coord) {
	return read_imageui(img, sampler, convert_int2(coord));
}

OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_float_nearest_hw(read_only image2d_t img, const float2 coord) {
	const sampler_t sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
	return read_imagef(img, sampler, coord);
}

// image write functions for native images
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(write_only image2d_t img, const uint2 coord, const float4 color) {
	write_imagef(img, convert_int2(coord), color);
}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(write_only image2d_t img, const uint2 coord, const int4 color) {
	write_imagei(img, convert_int2(coord), color);
}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(write_only image2d_t img, const uint2 coord, const uint4 color) {
	write_imageui(img, convert_int2(coord), color);
}

#endif

// image_read* and image_write* functions for buffer-based/software images
#include "oclr_image_support.h"

// the amd compiler doesn't need these workarounds and can simply use c++ to select the appropriate hardware or software image function
#if !defined(PLATFORM_AMD) && !defined(PLATFORM_POCL) && !defined(FLOOR_CUDA_CL)
// dummy image functions that are necessary for __builtin_choose_expr to function properly
// __builtin_choose_expr will do syntax checking on both expressions
// -> need to have fake sw/hw image functions with the resp. other type (sw taking image2d_t, hw taking mem ptrs)
OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_sw(read_only image2d_t img, const oclr_sampler_t sampler, const float2 coord) { return (float4)(0.0f); }
OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_sw(read_only image2d_t img, const oclr_sampler_t sampler, const uint2 coord) { return (float4)(0.0f); }
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_sw(read_only image2d_t img, const oclr_sampler_t sampler, const float2 coord) { return (int4)(0); }
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_sw(read_only image2d_t img, const oclr_sampler_t sampler, const uint2 coord) { return (int4)(0); }
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_sw(read_only image2d_t img, const oclr_sampler_t sampler, const float2 coord) { return (uint4)(0u); }
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_sw(read_only image2d_t img, const oclr_sampler_t sampler, const uint2 coord) { return (uint4)(0u); }
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_sw(write_only image2d_t img, const uint2 coord, const float4 color) {}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_sw(write_only image2d_t img, const uint2 coord, const int4 color) {}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_sw(write_only image2d_t img, const uint2 coord, const uint4 color) {}

OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_hw(global const void* img, const sampler_t sampler, const float2 coord) { return (float4)(0.0f); }
OCLRASTER_FUNC float4 FUNC_OVERLOAD image_read_hw(global const void* img, const sampler_t sampler, const uint2 coord) { return (float4)(0.0f); }
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_hw(global const void* img, const sampler_t sampler, const float2 coord) { return (int4)(0); }
OCLRASTER_FUNC int4 FUNC_OVERLOAD image_read_int_hw(global const void* img, const sampler_t sampler, const uint2 coord) { return (int4)(0); }
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_hw(global const void* img, const sampler_t sampler, const float2 coord) { return (uint4)(0u); }
OCLRASTER_FUNC uint4 FUNC_OVERLOAD image_read_uint_hw(global const void* img, const sampler_t sampler, const uint2 coord) { return (uint4)(0u); }
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(global void* img, const uint2 coord, const float4 color) {}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(global void* img, const uint2 coord, const int4 color) {}
OCLRASTER_FUNC void FUNC_OVERLOAD image_write_hw(global void* img, const uint2 coord, const uint4 color) {}

// now that both hw and sw image functions are defined, add image_read/image_write macros that will
// select the appropriate hw or sw image function depending on the argument type
#if !defined(PLATFORM_INTEL)
#define image_read(img, sampler, coord) \
__builtin_choose_expr(__alignof__(img) != 16, \
					  image_read_hw(img, sampler, coord), \
					  image_read_sw(img, sampler, coord))

#define image_read_int(img, sampler, coord) \
__builtin_choose_expr(__alignof__(img) != 16, \
					  image_read_int_hw(img, sampler, coord), \
					  image_read_int_sw(img, sampler, coord))

#define image_read_uint(img, sampler, coord) \
__builtin_choose_expr(__alignof__(img) != 16, \
					  image_read_uint_hw(img, sampler, coord), \
					  image_read_uint_sw(img, sampler, coord))
#else
// the intel opencl compiler explicitly needs a sampler_t variable
#define image_read(img, sampler, coord) \
({ \
	const sampler_t hw_sampler = sampler; \
	__builtin_choose_expr(__alignof__(img) != 16, \
						  image_read_hw(img, hw_sampler, coord), \
						  image_read_sw(img, sampler, coord)); \
})

#define image_read_int(img, sampler, coord) \
({ \
	const sampler_t hw_sampler = sampler; \
	__builtin_choose_expr(__alignof__(img) != 16, \
						  image_read_int_hw(img, hw_sampler, coord), \
						  image_read_int_sw(img, sampler, coord)); \
})

#define image_read_uint(img, sampler, coord) \
({ \
	const sampler_t hw_sampler = sampler; \
	__builtin_choose_expr(__alignof__(img) != 16, \
						  image_read_uint_hw(img, hw_sampler, coord), \
						  image_read_uint_sw(img, sampler, coord)); \
})
#endif

#define image_write(img, coord, color) \
__builtin_choose_expr(__alignof__(img) != 16, \
					  image_write_hw(img, coord, color), \
					  image_write_sw(img, coord, color))
//

#elif defined(PLATFORM_POCL)
// for pocl: simply forward to the software image functions
#define image_read(img, sampler, coord) image_read_sw(img, sampler, coord)
#define image_read_int(img, sampler, coord) image_read_int_sw(img, sampler, coord)
#define image_read_uint(img, sampler, coord) image_read_uint_sw(img, sampler, coord)
#define image_write(img, coord, color) image_write_sw(img, coord, color)

#else // amd opencl c++ and cuda c++
// ... and now for the proper c++ solution to this problem:
#if !defined(FLOOR_CUDA_CL)
// note: already included and preprocessed by cuda
#include "oclr_cpp.h"
#endif

template<class T>
struct is_native_image : integral_constant<bool,
										   is_same<image1d_t, typename remove_cv<T>::type>::value ||
										   is_same<image2d_t, typename remove_cv<T>::type>::value ||
										   is_same<image3d_t, typename remove_cv<T>::type>::value> {};

template <typename image_type, typename coord_type,
		  typename enable_if<!is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC float4 image_read(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_sw(img, sampler, coord);
}
template <typename image_type, typename coord_type,
		  typename enable_if<is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC float4 image_read(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_hw(img, sampler, coord);
}

template <typename image_type, typename coord_type,
		  typename enable_if<!is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC int4 image_read_int(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_int_sw(img, sampler, coord);
}
template <typename image_type, typename coord_type,
		  typename enable_if<is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC int4 image_read_int(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_int_hw(img, sampler, coord);
}

template <typename image_type, typename coord_type,
		  typename enable_if<!is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC uint4 image_read_uint(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_uint_sw(img, sampler, coord);
}
template <typename image_type, typename coord_type,
		  typename enable_if<is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC uint4 image_read_uint(image_type img, const oclr_sampler_t sampler, const coord_type coord) {
	return image_read_uint_hw(img, sampler, coord);
}

template <typename image_type, typename color_type,
		  typename enable_if<!is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC void image_read(image_type img, const uint2 coord, const color_type color) {
	return image_write_sw(img, coord, color);
}
template <typename image_type, typename color_type,
		  typename enable_if<is_native_image<image_type>::value, int>::type = 0>
OCLRASTER_FUNC void image_read(image_type img, const uint2 coord, const color_type color) {
	return image_write_hw(img, coord, color);
}

#endif

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
