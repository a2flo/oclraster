// NOTE: this is an automatically generated file!
// If you need to change anything in here, please have a look at etc/image_support/image_support.sh

#ifndef __OCLRASTER_IMAGE_SUPPORT_H__
#define __OCLRASTER_IMAGE_SUPPORT_H__

float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)) / 255.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)) / 255.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const uchar* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float texels[4] = {
 convert_float(native_texels[0]),
 convert_float(native_texels[1]),
 convert_float(native_texels[2]),
 convert_float(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 255.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)) / 255.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)) / 255.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const uchar2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float2 texels[4] = {
 convert_float2(native_texels[0]),
 convert_float2(native_texels[1]),
 convert_float2(native_texels[2]),
 convert_float2(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 255.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)) / 255.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)) / 255.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const uchar3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float3 texels[4] = {
 convert_float3(native_texels[0]),
 convert_float3(native_texels[1]),
 convert_float3(native_texels[2]),
 convert_float3(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 255.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)) / 255.0f) );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const uchar4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)) / 255.0f) );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const uchar4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float4 texels[4] = {
 convert_float4(native_texels[0]),
 convert_float4(native_texels[1]),
 convert_float4(native_texels[2]),
 convert_float4(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 255.0f) );
}
float4 FUNC_OVERLOAD image_read(global const uchar4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const uchar4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uchar* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint texels[4] = {
 convert_uint(native_texels[0]),
 convert_uint(native_texels[1]),
 convert_uint(native_texels[2]),
 convert_uint(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uchar2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint2 texels[4] = {
 convert_uint2(native_texels[0]),
 convert_uint2(native_texels[1]),
 convert_uint2(native_texels[2]),
 convert_uint2(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uchar3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint3 texels[4] = {
 convert_uint3(native_texels[0]),
 convert_uint3(native_texels[1]),
 convert_uint3(native_texels[2]),
 convert_uint3(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uchar4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uchar4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uchar4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uchar4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uchar4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint4 texels[4] = {
 convert_uint4(native_texels[0]),
 convert_uint4(native_texels[1]),
 convert_uint4(native_texels[2]),
 convert_uint4(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uchar4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)) / 65535.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)) / 65535.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const ushort* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float texels[4] = {
 convert_float(native_texels[0]),
 convert_float(native_texels[1]),
 convert_float(native_texels[2]),
 convert_float(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 65535.0f) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)) / 65535.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)) / 65535.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const ushort2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float2 texels[4] = {
 convert_float2(native_texels[0]),
 convert_float2(native_texels[1]),
 convert_float2(native_texels[2]),
 convert_float2(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 65535.0f) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)) / 65535.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)) / 65535.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const ushort3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float3 texels[4] = {
 convert_float3(native_texels[0]),
 convert_float3(native_texels[1]),
 convert_float3(native_texels[2]),
 convert_float3(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 65535.0f) , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)) / 65535.0f) );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const ushort4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)) / 65535.0f) );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const ushort4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float4 texels[4] = {
 convert_float4(native_texels[0]),
 convert_float4(native_texels[1]),
 convert_float4(native_texels[2]),
 convert_float4(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)) / 65535.0f) );
}
float4 FUNC_OVERLOAD image_read(global const ushort4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const ushort4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const ushort* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint texels[4] = {
 convert_uint(native_texels[0]),
 convert_uint(native_texels[1]),
 convert_uint(native_texels[2]),
 convert_uint(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const ushort2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint2 texels[4] = {
 convert_uint2(native_texels[0]),
 convert_uint2(native_texels[1]),
 convert_uint2(native_texels[2]),
 convert_uint2(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const ushort3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint3 texels[4] = {
 convert_uint3(native_texels[0]),
 convert_uint3(native_texels[1]),
 convert_uint3(native_texels[2]),
 convert_uint3(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ushort4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const ushort4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ushort4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const ushort4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ushort4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint4 texels[4] = {
 convert_uint4(native_texels[0]),
 convert_uint4(native_texels[1]),
 convert_uint4(native_texels[2]),
 convert_uint4(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const ushort4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uint texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uint texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint(texel))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uint* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uint native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint texels[4] = {
 convert_uint(native_texels[0]),
 convert_uint(native_texels[1]),
 convert_uint(native_texels[2]),
 convert_uint(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uint2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uint2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint2(texel))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uint2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uint2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint2 texels[4] = {
 convert_uint2(native_texels[0]),
 convert_uint2(native_texels[1]),
 convert_uint2(native_texels[2]),
 convert_uint2(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uint3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uint3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint3(texel))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uint3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uint3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint3 texels[4] = {
 convert_uint3(native_texels[0]),
 convert_uint3(native_texels[1]),
 convert_uint3(native_texels[2]),
 convert_uint3(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const uint4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_nearest(global const uint4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const uint4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (uint4)( ((convert_uint4(texel))) );
}
uint4 FUNC_OVERLOAD image_read_uint_linear(global const uint4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const uint4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const uint4 texels[4] = {
 convert_uint4(native_texels[0]),
 convert_uint4(native_texels[1]),
 convert_uint4(native_texels[2]),
 convert_uint4(native_texels[3]),
 };
 return (uint4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_uint_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}
uint4 FUNC_OVERLOAD image_read_uint(global const uint4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_uint_nearest(img, coord);
 return (uint4)(0, 0, 0, 1);
}


ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ulong texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong(texel))) , 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ulong texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong(texel))) , 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_linear(global const ulong* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ulong native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const ulong texels[4] = {
 convert_ulong(native_texels[0]),
 convert_ulong(native_texels[1]),
 convert_ulong(native_texels[2]),
 convert_ulong(native_texels[3]),
 };
 return (ulong4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_ulong_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}


ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ulong2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong2(texel))) , 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ulong2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong2(texel))) , 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_linear(global const ulong2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ulong2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const ulong2 texels[4] = {
 convert_ulong2(native_texels[0]),
 convert_ulong2(native_texels[1]),
 convert_ulong2(native_texels[2]),
 convert_ulong2(native_texels[3]),
 };
 return (ulong4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_ulong_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}


ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ulong3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong3(texel))) , 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ulong3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong3(texel))) , 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong_linear(global const ulong3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ulong3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const ulong3 texels[4] = {
 convert_ulong3(native_texels[0]),
 convert_ulong3(native_texels[1]),
 convert_ulong3(native_texels[2]),
 convert_ulong3(native_texels[3]),
 };
 return (ulong4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_ulong_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}


ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const ulong4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong4(texel))) );
}
ulong4 FUNC_OVERLOAD image_read_ulong_nearest(global const ulong4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const ulong4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (ulong4)( ((convert_ulong4(texel))) );
}
ulong4 FUNC_OVERLOAD image_read_ulong_linear(global const ulong4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const ulong4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const ulong4 texels[4] = {
 convert_ulong4(native_texels[0]),
 convert_ulong4(native_texels[1]),
 convert_ulong4(native_texels[2]),
 convert_ulong4(native_texels[3]),
 };
 return (ulong4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_ulong_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}
ulong4 FUNC_OVERLOAD image_read_ulong(global const ulong4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_ulong_nearest(img, coord);
 return (ulong4)(0, 0, 0, 1);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const char* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const char* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const char* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float texels[4] = {
 convert_float(native_texels[0]),
 convert_float(native_texels[1]),
 convert_float(native_texels[2]),
 convert_float(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const char2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const char2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const char2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float2 texels[4] = {
 convert_float2(native_texels[0]),
 convert_float2(native_texels[1]),
 convert_float2(native_texels[2]),
 convert_float2(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const char3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const char3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const char3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float3 texels[4] = {
 convert_float3(native_texels[0]),
 convert_float3(native_texels[1]),
 convert_float3(native_texels[2]),
 convert_float3(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 128.0f) / 255.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const char4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const char4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)+ 128.0f) / 255.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const char4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float4 texels[4] = {
 convert_float4(native_texels[0]),
 convert_float4(native_texels[1]),
 convert_float4(native_texels[2]),
 convert_float4(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 128.0f) / 255.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read(global const char4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const char4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const char* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const char* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const char* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int texels[4] = {
 convert_int(native_texels[0]),
 convert_int(native_texels[1]),
 convert_int(native_texels[2]),
 convert_int(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const char2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const char2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const char2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int2 texels[4] = {
 convert_int2(native_texels[0]),
 convert_int2(native_texels[1]),
 convert_int2(native_texels[2]),
 convert_int2(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const char3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const char3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const char3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int3 texels[4] = {
 convert_int3(native_texels[0]),
 convert_int3(native_texels[1]),
 convert_int3(native_texels[2]),
 convert_int3(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const char4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const char4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const char4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const char4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_linear(global const char4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const char4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int4 texels[4] = {
 convert_int4(native_texels[0]),
 convert_int4(native_texels[1]),
 convert_int4(native_texels[2]),
 convert_int4(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
int4 FUNC_OVERLOAD image_read_int(global const char4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const char4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const short* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const short* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const short* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float texels[4] = {
 convert_float(native_texels[0]),
 convert_float(native_texels[1]),
 convert_float(native_texels[2]),
 convert_float(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const short2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const short2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const short2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float2 texels[4] = {
 convert_float2(native_texels[0]),
 convert_float2(native_texels[1]),
 convert_float2(native_texels[2]),
 convert_float2(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const short3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const short3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const short3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float3 texels[4] = {
 convert_float3(native_texels[0]),
 convert_float3(native_texels[1]),
 convert_float3(native_texels[2]),
 convert_float3(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const short4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const short4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const short4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float4 texels[4] = {
 convert_float4(native_texels[0]),
 convert_float4(native_texels[1]),
 convert_float4(native_texels[2]),
 convert_float4(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y)+ 32768.0f) / 65535.0f) * 2.0f - 1.0f );
}
float4 FUNC_OVERLOAD image_read(global const short4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const short4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const short* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const short* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const short* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int texels[4] = {
 convert_int(native_texels[0]),
 convert_int(native_texels[1]),
 convert_int(native_texels[2]),
 convert_int(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const short2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const short2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const short2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int2 texels[4] = {
 convert_int2(native_texels[0]),
 convert_int2(native_texels[1]),
 convert_int2(native_texels[2]),
 convert_int2(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const short3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const short3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const short3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int3 texels[4] = {
 convert_int3(native_texels[0]),
 convert_int3(native_texels[1]),
 convert_int3(native_texels[2]),
 convert_int3(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const short4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const short4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const short4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const short4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_linear(global const short4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const short4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int4 texels[4] = {
 convert_int4(native_texels[0]),
 convert_int4(native_texels[1]),
 convert_int4(native_texels[2]),
 convert_int4(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
int4 FUNC_OVERLOAD image_read_int(global const short4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const short4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const int* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const int texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const int* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const int texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int(texel))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const int* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const int native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int texels[4] = {
 convert_int(native_texels[0]),
 convert_int(native_texels[1]),
 convert_int(native_texels[2]),
 convert_int(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const int2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const int2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const int2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const int2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int2(texel))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const int2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const int2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int2 texels[4] = {
 convert_int2(native_texels[0]),
 convert_int2(native_texels[1]),
 convert_int2(native_texels[2]),
 convert_int2(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const int3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const int3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const int3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const int3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int3(texel))) , 1);
}
int4 FUNC_OVERLOAD image_read_int_linear(global const int3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const int3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int3 texels[4] = {
 convert_int3(native_texels[0]),
 convert_int3(native_texels[1]),
 convert_int3(native_texels[2]),
 convert_int3(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


int4 FUNC_OVERLOAD image_read_int_nearest(global const int4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const int4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_nearest(global const int4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const int4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (int4)( ((convert_int4(texel))) );
}
int4 FUNC_OVERLOAD image_read_int_linear(global const int4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const int4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const int4 texels[4] = {
 convert_int4(native_texels[0]),
 convert_int4(native_texels[1]),
 convert_int4(native_texels[2]),
 convert_int4(native_texels[3]),
 };
 return (int4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
int4 FUNC_OVERLOAD image_read_int(global const int4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_int_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}
int4 FUNC_OVERLOAD image_read_int(global const int4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_int_nearest(img, coord);
 return (int4)(0, 0, 0, 1);
}


long4 FUNC_OVERLOAD image_read_long_nearest(global const long* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const long texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long(texel))) , 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long_nearest(global const long* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const long texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long(texel))) , 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long_linear(global const long* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const long native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const long texels[4] = {
 convert_long(native_texels[0]),
 convert_long(native_texels[1]),
 convert_long(native_texels[2]),
 convert_long(native_texels[3]),
 };
 return (long4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_long_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}


long4 FUNC_OVERLOAD image_read_long_nearest(global const long2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const long2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long2(texel))) , 0, 1);
}
long4 FUNC_OVERLOAD image_read_long_nearest(global const long2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const long2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long2(texel))) , 0, 1);
}
long4 FUNC_OVERLOAD image_read_long_linear(global const long2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const long2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const long2 texels[4] = {
 convert_long2(native_texels[0]),
 convert_long2(native_texels[1]),
 convert_long2(native_texels[2]),
 convert_long2(native_texels[3]),
 };
 return (long4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_long_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}


long4 FUNC_OVERLOAD image_read_long_nearest(global const long3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const long3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long3(texel))) , 1);
}
long4 FUNC_OVERLOAD image_read_long_nearest(global const long3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const long3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long3(texel))) , 1);
}
long4 FUNC_OVERLOAD image_read_long_linear(global const long3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const long3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const long3 texels[4] = {
 convert_long3(native_texels[0]),
 convert_long3(native_texels[1]),
 convert_long3(native_texels[2]),
 convert_long3(native_texels[3]),
 };
 return (long4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_long_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}


long4 FUNC_OVERLOAD image_read_long_nearest(global const long4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const long4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long4(texel))) );
}
long4 FUNC_OVERLOAD image_read_long_nearest(global const long4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const long4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (long4)( ((convert_long4(texel))) );
}
long4 FUNC_OVERLOAD image_read_long_linear(global const long4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const long4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const long4 texels[4] = {
 convert_long4(native_texels[0]),
 convert_long4(native_texels[1]),
 convert_long4(native_texels[2]),
 convert_long4(native_texels[3]),
 };
 return (long4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
long4 FUNC_OVERLOAD image_read_long(global const long4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_long_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}
long4 FUNC_OVERLOAD image_read_long(global const long4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_long_nearest(img, coord);
 return (long4)(0, 0, 0, 1);
}

float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float texel = vload_half(ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float texel = vload_half(coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const oclr_half* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float texels[4] = {
 vload_half(coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half(coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half(coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half(coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 };
 return (float4)(
 texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float2 texel = vload_half2(ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 texel = vload_half2(coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const oclr_half2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float2 texels[4] = {
 vload_half2(coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half2(coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half2(coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half2(coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 };
 return (float4)(
 texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float3 texel = vload_half3(ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float3 texel = vload_half3(coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const oclr_half3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float3 texels[4] = {
 vload_half3(coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half3(coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half3(coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half3(coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 };
 return (float4)(
 texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y) , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}

float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float4 texel = vload_half4(ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const oclr_half4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float4 texel = vload_half4(coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
 return (float4)(texel );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const oclr_half4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float4 texels[4] = {
 vload_half4(coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half4(coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half4(coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 vload_half4(coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
 };
 return (float4)(
 texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y) );
}
float4 FUNC_OVERLOAD image_read(global const oclr_half4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const oclr_half4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const float* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel))) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const float* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float(texel))) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const float* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float texels[4] = {
 convert_float(native_texels[0]),
 convert_float(native_texels[1]),
 convert_float(native_texels[2]),
 convert_float(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const float2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel))) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const float2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float2(texel))) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const float2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float2 texels[4] = {
 convert_float2(native_texels[0]),
 convert_float2(native_texels[1]),
 convert_float2(native_texels[2]),
 convert_float2(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const float3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel))) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const float3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float3(texel))) , 1.0f);
}
float4 FUNC_OVERLOAD image_read_float_linear(global const float3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float3 texels[4] = {
 convert_float3(native_texels[0]),
 convert_float3(native_texels[1]),
 convert_float3(native_texels[2]),
 convert_float3(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


float4 FUNC_OVERLOAD image_read_float_nearest(global const float4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const float4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel))) );
}
float4 FUNC_OVERLOAD image_read_float_nearest(global const float4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (float4)( ((convert_float4(texel))) );
}
float4 FUNC_OVERLOAD image_read_float_linear(global const float4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const float4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const float4 texels[4] = {
 convert_float4(native_texels[0]),
 convert_float4(native_texels[1]),
 convert_float4(native_texels[2]),
 convert_float4(native_texels[3]),
 };
 return (float4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
float4 FUNC_OVERLOAD image_read(global const float4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_float_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}
float4 FUNC_OVERLOAD image_read(global const float4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_float_nearest(img, coord);
 return (float4)(0.0f, 0.0f, 0.0f, 1.0f);
}


double4 FUNC_OVERLOAD image_read_double_nearest(global const double* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const double texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double(texel))) , 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double_nearest(global const double* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const double texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double(texel))) , 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double_linear(global const double* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const double native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const double texels[4] = {
 convert_double(native_texels[0]),
 convert_double(native_texels[1]),
 convert_double(native_texels[2]),
 convert_double(native_texels[3]),
 };
 return (double4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_double_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}


double4 FUNC_OVERLOAD image_read_double_nearest(global const double2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const double2 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double2(texel))) , 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double_nearest(global const double2* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const double2 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double2(texel))) , 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double_linear(global const double2* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const double2 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const double2 texels[4] = {
 convert_double2(native_texels[0]),
 convert_double2(native_texels[1]),
 convert_double2(native_texels[2]),
 convert_double2(native_texels[3]),
 };
 return (double4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double2* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_double_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double2* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}


double4 FUNC_OVERLOAD image_read_double_nearest(global const double3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const double3 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double3(texel))) , 1.0);
}
double4 FUNC_OVERLOAD image_read_double_nearest(global const double3* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const double3 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double3(texel))) , 1.0);
}
double4 FUNC_OVERLOAD image_read_double_linear(global const double3* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const double3 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const double3 texels[4] = {
 convert_double3(native_texels[0]),
 convert_double3(native_texels[1]),
 convert_double3(native_texels[2]),
 convert_double3(native_texels[3]),
 };
 return (double4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) , 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double3* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_double_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double3* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}


double4 FUNC_OVERLOAD image_read_double_nearest(global const double4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
 const double4 texel = img[ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double4(texel))) );
}
double4 FUNC_OVERLOAD image_read_double_nearest(global const double4* img, const uint2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const double4 texel = img[coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE];
 return (double4)( ((convert_double4(texel))) );
}
double4 FUNC_OVERLOAD image_read_double_linear(global const double4* img, const float2 coord) {
 const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
 const float2 fimg_size = convert_float2(img_size) - 1.0f;
 const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
 const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
 float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
 const float2 weights = scaled_coord - fcoords.xy;
 fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
 fimg_size.x, fimg_size.y));
 const uint4 coords = (uint4)((uint)fcoords.x,
 img_size.x * (uint)fcoords.y,
 (uint)fcoords.z,
 img_size.x * (uint)fcoords.w);
 const double4 native_texels[4] = {
 img[coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE],
 img[coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE]
 };
 const double4 texels[4] = {
 convert_double4(native_texels[0]),
 convert_double4(native_texels[1]),
 convert_double4(native_texels[2]),
 convert_double4(native_texels[3]),
 };
 return (double4)(
 ((texel_mix(texel_mix(texels[0], texels[1], weights.x),
 texel_mix(texels[2], texels[3], weights.x),
 weights.y))) );
}
double4 FUNC_OVERLOAD image_read_double(global const double4* img, const sampler_t sampler, const float2 coord) {
 if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return image_read_double_linear(img, coord);
 else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}
double4 FUNC_OVERLOAD image_read_double(global const double4* img, const sampler_t sampler, const uint2 coord) {
 if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return image_read_double_nearest(img, coord);
 return (double4)(0.0, 0.0, 0.0, 1.0);
}


#endif
