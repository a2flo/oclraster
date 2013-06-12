
#define IMG_READ_FUNC_FILTER_CONCAT(return_type, filter) image_read_##return_type##_##filter
#define IMG_READ_FUNC_FILTER_EVAL(return_type, filter) IMG_READ_FUNC_FILTER_CONCAT(return_type, filter)
#define IMG_READ_FUNC_FILTER_NAME(filter) IMG_READ_FUNC_FILTER_EVAL(RETURN_TYPE, filter)
#define IMG_READ_FUNC_CONCAT(return_name) image_read##return_name
#define IMG_READ_FUNC_EVAL(return_name) IMG_READ_FUNC_CONCAT(return_name)
#define IMG_READ_FUNC_NAME() IMG_READ_FUNC_EVAL(FUNC_RETURN_NAME)
#define IMG_WRITE_FUNC_CONCAT(return_name) image_write##return_name
#define IMG_WRITE_FUNC_EVAL(return_name) IMG_WRITE_FUNC_CONCAT(return_name)
#define IMG_WRITE_FUNC_NAME() IMG_WRITE_FUNC_EVAL(FUNC_RETURN_NAME)
#define HALF_VEC_LOAD_CONCAT(vecn) vload_half##vecn
#define HALF_VEC_LOAD_EVAL(vecn) HALF_VEC_LOAD_CONCAT(vecn)
#define HALF_VEC_LOAD HALF_VEC_LOAD_EVAL(VECN)
#define HALF_VEC_STORE_CONCAT(vecn) vstore_half##vecn
#define HALF_VEC_STORE_EVAL(vecn) HALF_VEC_STORE_CONCAT(vecn)
#define HALF_VEC_STORE HALF_VEC_STORE_EVAL(VECN)

/////////////////
// read functions
RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_FILTER_NAME(nearest)(global const IMG_TYPE* img, const uint offset) {
	global const half* img_data_ptr = (global const half*)((global const uchar*)img + OCLRASTER_IMAGE_HEADER_SIZE);
	const RETURN_TYPE_VEC texel = HALF_VEC_LOAD(offset, img_data_ptr);
	return (RETURN_TYPE_VEC4)(texel VEC4_FILL);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_FILTER_NAME(nearest)(global const IMG_TYPE* img, const float2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const float2 fimg_size = convert_float2(img_size) - 1.0f;

	// normalize input texture coordinate to [0, 1]
	const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
	const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
	return IMG_READ_FUNC_FILTER_NAME(nearest)(img, ui_tc.y * img_size.x + ui_tc.x);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_FILTER_NAME(nearest)(global const IMG_TYPE* img, const uint2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	return IMG_READ_FUNC_FILTER_NAME(nearest)(img, coord.y * img_size.x + coord.x);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_FILTER_NAME(linear)(global const IMG_TYPE* img, const float2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	global const half* img_data_ptr = (global const half*)((global const uchar*)img + OCLRASTER_IMAGE_HEADER_SIZE);
	const float2 fimg_size = convert_float2(img_size) - 1.0f;
	
	// normalize input texture coordinate to [0, 1]
	const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
	
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
	const RETURN_TYPE_VEC texels[4] = {
		// bilinear coords
		HALF_VEC_LOAD(coords.y + coords.x, img_data_ptr),
		HALF_VEC_LOAD(coords.y + coords.z, img_data_ptr),
		HALF_VEC_LOAD(coords.w + coords.x, img_data_ptr),
		HALF_VEC_LOAD(coords.w + coords.z, img_data_ptr),
	};
	return (RETURN_TYPE_VEC4)(
		texel_mix(texel_mix(texels[0], texels[1], weights.x),
				  texel_mix(texels[2], texels[3], weights.x),
				  weights.y) VEC4_FILL);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_NAME()(global const IMG_TYPE* img, const sampler_t sampler, const float2 coord) {
	// need to check linear first (CLK_FILTER_NEAREST might be 0)
	if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return IMG_READ_FUNC_FILTER_NAME(linear)(img, coord);
	else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return IMG_READ_FUNC_FILTER_NAME(nearest)(img, coord);
	return (RETURN_TYPE_VEC4)(IMG_ZERO, IMG_ZERO, IMG_ZERO, IMG_ONE);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD OCLRASTER_FUNC IMG_READ_FUNC_NAME()(global const IMG_TYPE* img, const sampler_t sampler, const uint2 coord) {
	// filter must be set to CLK_FILTER_NEAREST
	if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return IMG_READ_FUNC_FILTER_NAME(nearest)(img, coord);
	return (RETURN_TYPE_VEC4)(IMG_ZERO, IMG_ZERO, IMG_ZERO, IMG_ONE);
}

//////////////////
// write functions
void FUNC_OVERLOAD OCLRASTER_FUNC IMG_WRITE_FUNC_NAME()(global IMG_TYPE* img, const uint2 coord, const RETURN_TYPE_VEC4 color) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const uint offset = coord.y * img_size.x + coord.x;
	global IMG_TYPE* img_data_ptr = (global IMG_TYPE*)((global uchar*)img + OCLRASTER_IMAGE_HEADER_SIZE);
#if (CHANNEL_COUNT == 1)
#define IMG_COLOR color.x
#elif (CHANNEL_COUNT == 2)
#define IMG_COLOR color.xy
#elif (CHANNEL_COUNT == 3)
#define IMG_COLOR color.xyz
#elif (CHANNEL_COUNT == 4)
#define IMG_COLOR color.xyzw
#endif
	HALF_VEC_STORE(IMG_COLOR, offset, img_data_ptr);
}
