
#if (CHANNEL_COUNT == 1)
#define VSTORE_FUNC_NAME_CONCAT(channel_count) vstore_half
#else
#define VSTORE_FUNC_NAME_CONCAT(channel_count) vstore_half##channel_count
#endif
#define VSTORE_FUNC_NAME_EVAL(channel_count) VSTORE_FUNC_NAME_CONCAT(channel_count)
#define VSTORE_FUNC_NAME() VSTORE_FUNC_NAME_EVAL(CHANNEL_COUNT)

#if (CHANNEL_COUNT == 1)
#define CLEAR_COLOR_COMP clear_color.x
#elif (CHANNEL_COUNT == 2)
#define CLEAR_COLOR_COMP clear_color.xy
#elif (CHANNEL_COUNT == 3)
#define CLEAR_COLOR_COMP clear_color.xyz
#elif (CHANNEL_COUNT == 4)
#define CLEAR_COLOR_COMP clear_color
#endif

#if (IS_HALF_TYPE == 0)
#define CLEAR_TYPE_VEC_CONCAT(img_type, channel_count) img_type##channel_count
#define CLEAR_TYPE_VEC_EVAL(img_type) CLEAR_TYPE_VEC_CONCAT(img_type, 4)
#define CLEAR_TYPE_VEC CLEAR_TYPE_VEC_EVAL(IMG_TYPE)
#else
#define CLEAR_TYPE_VEC float4
#endif

void FUNC_OVERLOAD OCLRASTER_FUNC clear_image(global IMG_TYPE_VEC* img, const uint offset, const CLEAR_TYPE_VEC clear_color) {
#if (IS_HALF_TYPE == 0)
	img[offset] = CLEAR_COLOR_COMP;
#else
	// half image types need special treatment (store via vstore with float values)
	VSTORE_FUNC_NAME()(CLEAR_COLOR_COMP, offset, (global half*)img);
#endif
}
