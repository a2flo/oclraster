// NOTE: this is an automatically generated file!
// If you need to change anything in here, please have a look at etc/image_support/image_support.sh

#ifndef __OCLRASTER_FRAMEBUFFER_CLEAR_H__
#define __OCLRASTER_FRAMEBUFFER_CLEAR_H__

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uchar* img, const uint offset, const uchar4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uchar2* img, const uint offset, const uchar4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uchar3* img, const uint offset, const uchar4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uchar4* img, const uint offset, const uchar4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ushort* img, const uint offset, const ushort4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ushort2* img, const uint offset, const ushort4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ushort3* img, const uint offset, const ushort4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ushort4* img, const uint offset, const ushort4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uint* img, const uint offset, const uint4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uint2* img, const uint offset, const uint4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uint3* img, const uint offset, const uint4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global uint4* img, const uint offset, const uint4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ulong* img, const uint offset, const ulong4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ulong2* img, const uint offset, const ulong4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ulong3* img, const uint offset, const ulong4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global ulong4* img, const uint offset, const ulong4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global char* img, const uint offset, const char4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global char2* img, const uint offset, const char4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global char3* img, const uint offset, const char4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global char4* img, const uint offset, const char4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global short* img, const uint offset, const short4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global short2* img, const uint offset, const short4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global short3* img, const uint offset, const short4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global short4* img, const uint offset, const short4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global int* img, const uint offset, const int4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global int2* img, const uint offset, const int4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global int3* img, const uint offset, const int4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global int4* img, const uint offset, const int4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global long* img, const uint offset, const long4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global long2* img, const uint offset, const long4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global long3* img, const uint offset, const long4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global long4* img, const uint offset, const long4 clear_color) {
 img[offset] = clear_color;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global oclr_half* img, const uint offset, const float4 clear_color) {
 vstore_half(clear_color.x, offset, (global half*)img);
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global oclr_half2* img, const uint offset, const float4 clear_color) {
 vstore_half2(clear_color.xy, offset, (global half*)img);
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global oclr_half3* img, const uint offset, const float4 clear_color) {
 vstore_half3(clear_color.xyz, offset, (global half*)img);
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global oclr_half4* img, const uint offset, const float4 clear_color) {
 vstore_half4(clear_color, offset, (global half*)img);
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global float* img, const uint offset, const float4 clear_color) {
 img[offset] = clear_color.x;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global float2* img, const uint offset, const float4 clear_color) {
 img[offset] = clear_color.xy;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global float3* img, const uint offset, const float4 clear_color) {
 img[offset] = clear_color.xyz;
}

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global float4* img, const uint offset, const float4 clear_color) {
 img[offset] = clear_color;
}

#if defined(OCLRASTER_DOUBLE_SUPPORT)
void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global double* img, const uint offset, const double4 clear_color) {
 img[offset] = clear_color.x;
}
#endif

#if defined(OCLRASTER_DOUBLE_SUPPORT)
void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global double2* img, const uint offset, const double4 clear_color) {
 img[offset] = clear_color.xy;
}
#endif

#if defined(OCLRASTER_DOUBLE_SUPPORT)
void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global double3* img, const uint offset, const double4 clear_color) {
 img[offset] = clear_color.xyz;
}
#endif

#if defined(OCLRASTER_DOUBLE_SUPPORT)
void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global double4* img, const uint offset, const double4 clear_color) {
 img[offset] = clear_color;
}
#endif


#endif
