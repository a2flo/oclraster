
#ifndef __FLOOR_CUDA_BASE_H__
#define __FLOOR_CUDA_BASE_H__

// defines
#define FLOOR_FUNC inline __device__
#define OCLRASTER_FUNC FLOOR_FUNC

// additional vector types
typedef struct {
	float4 lo;
	float4 hi;
} float8;
typedef struct {
	float8 lo;
	float8 hi;
} float16;
typedef struct {
	uint4 lo;
	uint4 hi;
} uint8;
typedef struct {
	uint8 lo;
	uint8 hi;
} uint16;
typedef struct {
	ulong4 lo;
	ulong4 hi;
} ulong8;
typedef struct {
	ulong8 lo;
	ulong8 hi;
} ulong16;

// include generated vector helper templates
#include "floor_cuda_vector_helper.hpp"

// clamp
// base case, single component: clamp(val, min, max)
template<class src_type, class dst_type,
		 typename enable_if<vector_mapping<src_type, 1>::src_vec_size == 1, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
FLOOR_FUNC dst_type clamp(const src_type src, const dst_type min_val, const dst_type max_val) {
	const dst_type conv_src = (dst_type)src;
	// max(src, min_val)
	const dst_type min_clamp = (conv_src > min_val ? conv_src : min_val);
	// min(_, max_val)
	return (min_clamp < max_val ? min_clamp : max_val);
}

// vector + single case: clamp(vecn, min, max)
template<class src_typen, class dst_type,
		 class dst_typen = typename vector_mapping<dst_type, 2>::type,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 2, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val, max_val);
	ret.y = clamp(src.y, min_val, max_val);
	return ret;
}
template<class src_typen, class dst_type,
		 class dst_typen = typename vector_mapping<dst_type, 3>::type,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 3, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val, max_val);
	ret.y = clamp(src.y, min_val, max_val);
	ret.z = clamp(src.z, min_val, max_val);
	return ret;
}
template<class src_typen, class dst_type,
		 class dst_typen = typename vector_mapping<dst_type, 4>::type,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 4, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val, max_val);
	ret.y = clamp(src.y, min_val, max_val);
	ret.z = clamp(src.z, min_val, max_val);
	ret.w = clamp(src.w, min_val, max_val);
	return ret;
}

// vector case: clamp(vecn, minn, maxn)
template<class src_typen, class dst_typen,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 2, int>::type = 0,
		 typename enable_if<vector_mapping<dst_typen, 1>::src_vec_size == 2, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	return ret;
}
template<class src_typen, class dst_typen,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 3, int>::type = 0,
		 typename enable_if<vector_mapping<dst_typen, 1>::src_vec_size == 3, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	ret.z = clamp(src.z, min_val.z, max_val.z);
	return ret;
}
template<class src_typen, class dst_typen,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 4, int>::type = 0,
		 typename enable_if<vector_mapping<dst_typen, 1>::src_vec_size == 4, int>::type = 0>
FLOOR_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	ret.z = clamp(src.z, min_val.z, max_val.z);
	ret.w = clamp(src.w, min_val.w, max_val.w);
	return ret;
}

// include generated cuda type conversion templates
#include "floor_cuda_conversion.hpp"

#endif
