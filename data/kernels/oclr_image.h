
#ifndef __OCLRASTER_IMAGE_H__
#define __OCLRASTER_IMAGE_H__

#include "oclr_global.h"

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

//
typedef struct __attribute__((packed, aligned(32))) {
	const ushort type;
	const ushort channel_order;
	const ushort width;
	const ushort height;
	const unsigned char _unused[24];
} image_header;
typedef global const image_header* image_header_ptr;

//
uint2 oclr_get_image_size(global const image_header* img) {
	return (uint2)(img->width, img->height);
}
unsigned int oclr_get_image_type(global const image_header* img) {
	return img->type;
}
unsigned int oclr_get_image_channel_order(global const image_header* img) {
	return img->channel_order;
}

//
float FUNC_OVERLOAD texel_mix(float x, float y, float a) { return mix(x, y, a); }
float2 FUNC_OVERLOAD texel_mix(float2 x, float2 y, float a) { return mix(x, y, a); }
float3 FUNC_OVERLOAD texel_mix(float3 x, float3 y, float a) { return mix(x, y, a); }
float4 FUNC_OVERLOAD texel_mix(float4 x, float4 y, float a) { return mix(x, y, a); }
double FUNC_OVERLOAD texel_mix(double x, double y, float a) { return mix(x, y, (double)a); }
double2 FUNC_OVERLOAD texel_mix(double2 x, double2 y, float a) { return mix(x, y, (double)a); }
double3 FUNC_OVERLOAD texel_mix(double3 x, double3 y, float a) { return mix(x, y, (double)a); }
double4 FUNC_OVERLOAD texel_mix(double4 x, double4 y, float a) { return mix(x, y, (double)a); }

#define INT_TEXEL_MIX(type, type_vec, ftype, one) \
type_vec FUNC_OVERLOAD texel_mix(type_vec x, type_vec y, float a) { \
	const ftype inv_a = one / (ftype)a; \
	const ftype inv_b = one / (one - (ftype)a); \
	return (x / (type)inv_a) + (y / (type)inv_b); \
}
INT_TEXEL_MIX(uint, uint, float, 1.0f)
INT_TEXEL_MIX(uint, uint2, float, 1.0f)
INT_TEXEL_MIX(uint, uint3, float, 1.0f)
INT_TEXEL_MIX(uint, uint4, float, 1.0f)
INT_TEXEL_MIX(ulong, ulong, double, 1.0)
INT_TEXEL_MIX(ulong, ulong2, double, 1.0)
INT_TEXEL_MIX(ulong, ulong3, double, 1.0)
INT_TEXEL_MIX(ulong, ulong4, double, 1.0)
INT_TEXEL_MIX(int, int, float, 1.0f)
INT_TEXEL_MIX(int, int2, float, 1.0f)
INT_TEXEL_MIX(int, int3, float, 1.0f)
INT_TEXEL_MIX(int, int4, float, 1.0f)
INT_TEXEL_MIX(long, long, double, 1.0)
INT_TEXEL_MIX(long, long2, double, 1.0)
INT_TEXEL_MIX(long, long3, double, 1.0)
INT_TEXEL_MIX(long, long4, double, 1.0)

// image_read (implicit float) -> image_read_float_nearest / image_read_float_linear
// image_read_int -> image_read_int_nearest / image_read_int_linear
// image_read_uint -> image_read_uint_nearest / image_read_uint_linear
#include "oclr_image_support.h"

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
