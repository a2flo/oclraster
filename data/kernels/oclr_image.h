
#ifndef __OCLRASTER_IMAGE_H__
#define __OCLRASTER_IMAGE_H__

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

//
typedef struct __attribute__((packed, aligned(16))) {
	const unsigned int type;
	const unsigned int channel_order;
	const unsigned int width;
	const unsigned int height;
} image_header;
#define IMAGE_HEADER_SIZE (sizeof(image_header))
typedef global const image_header* image_header_ptr;

// image typedefs
//#define IMAGE_ALIGNMENT __attribute__((aligned(4)))
#define IMAGE_ALIGNMENT
typedef global const char* IMAGE_ALIGNMENT int8_image;
typedef global const short* IMAGE_ALIGNMENT int16_image;
typedef global const int* IMAGE_ALIGNMENT int32_image;
typedef global const long* IMAGE_ALIGNMENT int64_image;
typedef global const uchar* IMAGE_ALIGNMENT uint8_image;
typedef global const ushort* IMAGE_ALIGNMENT uint16_image;
typedef global const uint* IMAGE_ALIGNMENT uint32_image;
typedef global const ulong* IMAGE_ALIGNMENT uint64_image;
typedef global const half* IMAGE_ALIGNMENT float16_image;
typedef global const float* IMAGE_ALIGNMENT float32_image;
typedef global const double* IMAGE_ALIGNMENT float64_image;

//
const uint2 oclr_get_image_size(global const image_header* img) {
	return (uint2)(img->width, img->height);
}
const unsigned int oclr_get_image_type(global const image_header* img) {
	return img->type;
}
const unsigned int oclr_get_image_channel_order(global const image_header* img) {
	return img->channel_order;
}

//
float4 oclr_read_image(uint32_image img, const float2 tex_coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const float2 fimg_size = convert_float2(img_size) - 1.0f;
	
	const uint2 ui_tc = clamp(convert_uint2(tex_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
	const unsigned int texel = img[ui_tc.y * img_size.x + ui_tc.x + IMAGE_HEADER_SIZE];
	const uchar4 u4_texel = *(const uchar4*)&texel;
	return convert_float4(u4_texel) / 255.0f;
}

float4 oclr_read_image_lin(uint32_image img, const float2 tex_coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const float2 fimg_size = convert_float2(img_size) - 1.0f;
	
	// normalize input texture coordinate to [0, 1]
	const float2 norm_coord = fmod(tex_coord + fabs(floor(tex_coord)), (float2)(1.0f, 1.0f));
	// compute texel coordinates for the 4 samples
	const float2 scaled_coord = norm_coord * fimg_size + 0.5f;
	float4 fcoords = (float4)(trunc(scaled_coord), ceil(scaled_coord));
	const float2 weights = scaled_coord - fcoords.xy;
	fcoords = fmod(fcoords, (float4)(fimg_size.x, fimg_size.y,
									 fimg_size.x, fimg_size.y));
	
	const uint4 coords = (uint4)((uint)fcoords.x,
								 img_size.x * (uint)fcoords.y,
								 (uint)fcoords.z,
								 img_size.x * (uint)fcoords.w);
	
	// finally: read texels and interpolate according to weights
	const unsigned int ui_texels[4] = {
		img[coords.y + coords.x + IMAGE_HEADER_SIZE], // bilinear_coords
		img[coords.y + coords.z + IMAGE_HEADER_SIZE],
		img[coords.w + coords.x + IMAGE_HEADER_SIZE],
		img[coords.w + coords.z + IMAGE_HEADER_SIZE]
	};
	const float4 texels[4] = {
		convert_float4(*(const uchar4*)&ui_texels[0]),
		convert_float4(*(const uchar4*)&ui_texels[1]),
		convert_float4(*(const uchar4*)&ui_texels[2]),
		convert_float4(*(const uchar4*)&ui_texels[3]),
	};
	return mix(mix(texels[0], texels[1], weights.x),
			   mix(texels[2], texels[3], weights.x),
			   weights.y) / (float4)(255.0f);
}

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
