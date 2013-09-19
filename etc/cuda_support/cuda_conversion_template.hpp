
#define MAKE_CONV_FUNC_CONCAT(src_type, dst_type, round_mode) __##src_type##2##dst_type##_##round_mode
#define MAKE_CONV_FUNC_EVAL(src_type, dst_type, round_mode) MAKE_CONV_FUNC_CONCAT(src_type, dst_type, round_mode)
#define MAKE_CONV_FUNC() MAKE_CONV_FUNC_EVAL(CONV_SRC_TYPE, CONV_DST_TYPE, ROUND_MODE)
#define INPUT_CONV() INPUT_CONVERT

#if (IDENT_CONVERT == 0)
#define OUTPUT_CONV() (DST_TYPE)
#else
#define OUTPUT_CONV()
#endif

#if (CLAMP_HIGH != 0) && (IDENT_CONVERT == 0) && (DST_SIZE < SRC_SIZE) // saturated convert (clamp)
#define SAT_CONVERT(val) clamp(val, CLAMP_LOW, CLAMP_HIGH)
#else // no clamping necessary, as this is the largest possible type
#define SAT_CONVERT(val) val
#endif

#if (IDENT_CONVERT == 0) && (IDENT_KIND_CONVERT == 0)

#if (IDENT_KIND_CONVERT == 0)
template<> FLOOR_FUNC DST_TYPE convert_cuda_type<DST_TYPE, 0, SRC_TYPE>(const SRC_TYPE val) { return OUTPUT_CONV()MAKE_CONV_FUNC()(INPUT_CONV()val); }
template<> FLOOR_FUNC DST_TYPE convert_cuda_type<DST_TYPE, 1, SRC_TYPE>(const SRC_TYPE val) { return OUTPUT_CONV()SAT_CONVERT(MAKE_CONV_FUNC()(INPUT_CONV()val)); }
#else
#endif

#elif (IDENT_CONVERT == 1) || (IDENT_KIND_CONVERT == 1)

template<> FLOOR_FUNC DST_TYPE convert_cuda_type<DST_TYPE, 0, SRC_TYPE>(const SRC_TYPE val) { return OUTPUT_CONV()val; }
template<> FLOOR_FUNC DST_TYPE convert_cuda_type<DST_TYPE, 1, SRC_TYPE>(const SRC_TYPE val) { return OUTPUT_CONV()SAT_CONVERT(val); }

#endif
