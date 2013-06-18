
#ifndef __OCLRASTER_CUDA_BASE_H__
#define __OCLRASTER_CUDA_BASE_H__

// defines
#define OCLRASTER_FUNC inline __device__

// types
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned long int ulong;
typedef ushort half;

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

// c++ type support (copies the c++11 stl implementation, as it's not available here)
template<class T> struct remove_const { typedef T type; };
template<class T> struct remove_const<const T> { typedef T type; };
template<class T> struct remove_volatile { typedef T type; };
template<class T> struct remove_volatile<volatile T> { typedef T type; };
template<class T> struct remove_cv {
	typedef typename remove_volatile<typename remove_const<T>::type>::type type;
};

template<class T, T v>
struct integral_constant {
	static const T value = v;
	typedef T value_type;
	typedef integral_constant type;
};
typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template<class T, class U> struct is_same : false_type {};
template<class T> struct is_same<T, T> : true_type {};

template<bool B, class T = void> struct enable_if {};
template<class T> struct enable_if<true, T> { typedef T type; };

template<class T>
struct is_floating_point : integral_constant<bool,
											 is_same<float, typename remove_cv<T>::type>::value ||
											 is_same<double, typename remove_cv<T>::type>::value> {};

template<class T>
struct is_integer : integral_constant<bool,
									  is_same<bool, typename remove_cv<T>::type>::value ||
									  is_same<char, typename remove_cv<T>::type>::value ||
									  is_same<short, typename remove_cv<T>::type>::value ||
									  is_same<int, typename remove_cv<T>::type>::value ||
									  is_same<long, typename remove_cv<T>::type>::value ||
									  is_same<long long, typename remove_cv<T>::type>::value ||
									  is_same<unsigned char, typename remove_cv<T>::type>::value ||
									  is_same<unsigned short, typename remove_cv<T>::type>::value ||
									  is_same<unsigned int, typename remove_cv<T>::type>::value ||
									  is_same<unsigned long, typename remove_cv<T>::type>::value ||
									  is_same<unsigned long long, typename remove_cv<T>::type>::value> {};

// include generated vector helper templates
#include "oclr_cuda_vector_helper.h"

// clamp
// base case, single component: clamp(val, min, max)
template<class src_type, class dst_type,
		 typename enable_if<vector_mapping<src_type, 1>::src_vec_size == 1, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
OCLRASTER_FUNC dst_type clamp(const src_type src, const dst_type min_val, const dst_type max_val) {
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
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val, max_val);
	ret.y = clamp(src.y, min_val, max_val);
	return ret;
}
template<class src_typen, class dst_type,
		 class dst_typen = typename vector_mapping<dst_type, 3>::type,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 3, int>::type = 0,
		 typename enable_if<vector_mapping<dst_type, 1>::src_vec_size == 1, int>::type = 0>
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
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
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_type min_val, const dst_type max_val) {
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
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	return ret;
}
template<class src_typen, class dst_typen,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 3, int>::type = 0,
		 typename enable_if<vector_mapping<dst_typen, 1>::src_vec_size == 3, int>::type = 0>
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	ret.z = clamp(src.z, min_val.z, max_val.z);
	return ret;
}
template<class src_typen, class dst_typen,
		 typename enable_if<vector_mapping<src_typen, 1>::src_vec_size == 4, int>::type = 0,
		 typename enable_if<vector_mapping<dst_typen, 1>::src_vec_size == 4, int>::type = 0>
OCLRASTER_FUNC dst_typen clamp(const src_typen src, const dst_typen min_val, const dst_typen max_val) {
	dst_typen ret;
	ret.x = clamp(src.x, min_val.x, max_val.x);
	ret.y = clamp(src.y, min_val.y, max_val.y);
	ret.z = clamp(src.z, min_val.z, max_val.z);
	ret.w = clamp(src.w, min_val.w, max_val.w);
	return ret;
}

// include generated cuda type conversion templates
#include "oclr_cuda_conversion.h"

#endif
