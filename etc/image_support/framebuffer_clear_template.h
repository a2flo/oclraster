
#define CONVERT_FUNC_NAME_CONCAT(img_type) convert_##img_type##_sat
#define CONVERT_FUNC_NAME_EVAL(img_type) CONVERT_FUNC_NAME_CONCAT(img_type)
#define CONVERT_FUNC_NAME() CONVERT_FUNC_NAME_EVAL(IMG_TYPE_VEC)

#if (CHANNEL_COUNT == 1)
#define VSTORE_FUNC_NAME_CONCAT(channel_count) vstore_half
#else
#define VSTORE_FUNC_NAME_CONCAT(channel_count) vstore_half##channel_count
#endif
#define VSTORE_FUNC_NAME_EVAL(channel_count) VSTORE_FUNC_NAME_CONCAT(channel_count)
#define VSTORE_FUNC_NAME() VSTORE_FUNC_NAME_EVAL(CHANNEL_COUNT)

#if (CHANNEL_COUNT == 1)
#define AS_TYPEN_CONCAT(type, channel_count) as_##type
#else
#define AS_TYPEN_CONCAT(type, channel_count) as_##type##channel_count
#endif
#define AS_TYPEN_EVAL(type, channel_count) AS_TYPEN_CONCAT(type, channel_count)
#define AS_TYPEN(type) AS_TYPEN_EVAL(type, CHANNEL_COUNT)

#if (CHANNEL_COUNT == 1)
#define UINT_N_CONCAT(channel_count) convert_uint
#else
#define UINT_N_CONCAT(channel_count) convert_uint##channel_count
#endif
#define UINT_N_EVAL(channel_count) UINT_N_CONCAT(channel_count)
#define UINT_N() UINT_N_EVAL(CHANNEL_COUNT)

#if (CHANNEL_COUNT == 1)
#define CLEAR_COLOR_COMP clear_color.x
#elif (CHANNEL_COUNT == 2)
#define CLEAR_COLOR_COMP clear_color.xy
#elif (CHANNEL_COUNT == 3)
#define CLEAR_COLOR_COMP clear_color.xyz
#elif (CHANNEL_COUNT == 4)
#define CLEAR_COLOR_COMP clear_color.xyzw
#endif


void FUNC_OVERLOAD clear_image(global IMG_TYPE_VEC* img, const uint offset, const ulong4 clear_color) {
#if (IS_HALF_TYPE == 0) && (IS_FLOAT_TYPE == 0) && (IS_DOUBLE_TYPE == 0)
	// for unsigned and signed integer formats simply do a saturated convert:
	img[offset] = CONVERT_FUNC_NAME()(CLEAR_COLOR_COMP);
	
#else
	// for fp formats do a as_type reinterpretation:
#if (IS_HALF_TYPE == 1)
	// half
	VSTORE_FUNC_NAME()(AS_TYPEN(float)(UINT_N()(CLEAR_COLOR_COMP & 0xFFFFFFFFu)), offset, (global half*)img);
#elif (IS_FLOAT_TYPE == 1)
	// float
	img[offset] = AS_TYPEN(float)(UINT_N()(CLEAR_COLOR_COMP & 0xFFFFFFFFu));
#else
	// double
	img[offset] = AS_TYPEN(double)(CLEAR_COLOR_COMP);
#endif
	
#endif
}
