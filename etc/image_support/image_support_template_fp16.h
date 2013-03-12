
#define IMG_FUNC_FILTER_CONCAT(return_type, filter) image_read_##return_type##_##filter
#define IMG_FUNC_FILTER_EVAL(return_type, filter) IMG_FUNC_FILTER_CONCAT(return_type, filter)
#define IMG_FUNC_FILTER_NAME(filter) IMG_FUNC_FILTER_EVAL(RETURN_TYPE, filter)
#define IMG_FUNC_CONCAT(return_name) image_read##return_name
#define IMG_FUNC_EVAL(return_name) IMG_FUNC_CONCAT(return_name)
#define IMG_FUNC_NAME() IMG_FUNC_EVAL(FUNC_RETURN_NAME)
#define HALF_VEC_LOAD_CONCAT(vecn) vload_half##vecn
#define HALF_VEC_LOAD_EVAL(vecn) HALF_VEC_LOAD_CONCAT(vecn)
#define HALF_VEC_LOAD HALF_VEC_LOAD_EVAL(VECN)

RETURN_TYPE_VEC4 FUNC_OVERLOAD IMG_FUNC_FILTER_NAME(nearest)(global const IMG_TYPE* img, const float2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const float2 fimg_size = convert_float2(img_size) - 1.0f;
	
	// normalize input texture coordinate to [0, 1]
	const float2 norm_coord = fmod(coord + fabs(floor(coord)), (float2)(1.0f, 1.0f));
	
	const uint2 ui_tc = clamp(convert_uint2(norm_coord * fimg_size), (uint2)(0u, 0u), img_size - 1u);
	const RETURN_TYPE_VEC texel = HALF_VEC_LOAD(ui_tc.y * img_size.x + ui_tc.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
	return (RETURN_TYPE_VEC4)(texel VEC4_FILL);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD IMG_FUNC_FILTER_NAME(nearest)(global const IMG_TYPE* img, const uint2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
	const RETURN_TYPE_VEC texel = HALF_VEC_LOAD(coord.y * img_size.x + coord.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img);
	return (RETURN_TYPE_VEC4)(texel VEC4_FILL);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD IMG_FUNC_FILTER_NAME(linear)(global const IMG_TYPE* img, const float2 coord) {
	const uint2 img_size = oclr_get_image_size((image_header_ptr)img);
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
		HALF_VEC_LOAD(coords.y + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
		HALF_VEC_LOAD(coords.y + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
		HALF_VEC_LOAD(coords.w + coords.x + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
		HALF_VEC_LOAD(coords.w + coords.z + OCLRASTER_IMAGE_HEADER_SIZE, (global const half*)img),
	};
	return (RETURN_TYPE_VEC4)(
		texel_mix(texel_mix(texels[0], texels[1], weights.x),
				  texel_mix(texels[2], texels[3], weights.x),
				  weights.y) VEC4_FILL);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD IMG_FUNC_NAME()(global const IMG_TYPE* img, const sampler_t sampler, const float2 coord) {
	// need to check linear first (CLK_FILTER_NEAREST might be 0)
	if((sampler & CLK_FILTER_LINEAR) == CLK_FILTER_LINEAR) return IMG_FUNC_FILTER_NAME(linear)(img, coord);
	else if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return IMG_FUNC_FILTER_NAME(nearest)(img, coord);
	return (RETURN_TYPE_VEC4)(IMG_ZERO, IMG_ZERO, IMG_ZERO, IMG_ONE);
}

RETURN_TYPE_VEC4 FUNC_OVERLOAD IMG_FUNC_NAME()(global const IMG_TYPE* img, const sampler_t sampler, const uint2 coord) {
	// filter must be set to CLK_FILTER_NEAREST
	if((sampler & CLK_FILTER_NEAREST) == CLK_FILTER_NEAREST) return IMG_FUNC_FILTER_NAME(nearest)(img, coord);
	return (RETURN_TYPE_VEC4)(IMG_ZERO, IMG_ZERO, IMG_ZERO, IMG_ONE);
}
