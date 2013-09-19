
#ifndef __FLOOR_CUDA_VECTOR_BASE_H__
#define __FLOOR_CUDA_VECTOR_BASE_H__

// opencl type aliases
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
typedef ushort half;

// necessary cuda defines
#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long int size_t;
#endif
#if defined(__cplusplus)
namespace std {
	using ::size_t;
};
#endif

#define cuda_device __attribute__((always_inline, __device__))
//#define cuda_device_builtin __attribute__((__device_builtin__))
#define cuda_device_builtin
#define cuda_builtin_typedef __attribute__((__device_builtin__))

#if defined(__cplusplus)
#include "oclr_cpp.hpp"

//
#define TEMPLATE_ENABLE_IF_FLOAT class T, typename enable_if<is_same<float, T>::value == 1, int>::type = 0
#define TEMPLATE_ENABLE_IF_DOUBLE class T, typename enable_if<is_same<double, T>::value == 1, int>::type = 0

template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T sqrt_T(const T& val) {
	return sqrtf(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T sqrt_T(const T& val) {
	return sqrt(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T rsqrt_T(const T& val) {
	return rsqrtf(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T rsqrt_T(const T& val) {
	return rsqrt(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T fast_sqrt_T(const T& val) {
	return __fsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T fast_sqrt_T(const T& val) {
	return __dsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T fast_rsqrt_T(const T& val) {
	return __frsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T fast_rsqrt_T(const T& val) {
	return __drsqrt_rn(val);
}

#endif

#endif
