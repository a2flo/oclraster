
#ifndef __FLOOR_CUDA_VECTOR1_H__
#define __FLOOR_CUDA_VECTOR1_H__

#if defined(__cplusplus)

#undef CUDA_VECTOR_WIDTH
#define CUDA_VECTOR_WIDTH 1
#include "floor_cuda_vector_ops.hpp"

template <class T> struct cuda_device_builtin vector1 {
	T x;
	
	// the empty default constructor must not initialize any member variables,
	// so that it can be used for shared/local memory variables!
	cuda_device vector1() {}
	cuda_device vector1(const vector1<T>& vec) : x(vec.x) {}
	cuda_device vector1(const T& val) : x(val) {}
	
	cuda_device vector1& operator=(const vector1& vec) {
		this->x = vec.x;
		return *this;
	}
	cuda_device vector1& operator=(const T& val) {
		this->x = val;
		return *this;
	}
	
	//
	CUDA_VEC_OP(+)
	CUDA_VEC_OP(-)
	CUDA_VEC_OP(*)
	CUDA_VEC_OP(/)
	CUDA_VEC_OP(|)
	CUDA_VEC_OP(&)
	CUDA_VEC_OP(^)
	CUDA_VEC_UNARY_OP(+)
	CUDA_VEC_UNARY_OP(-)
	CUDA_VEC_UNARY_OP(!)
	CUDA_VEC_UNARY_OP(~)
	
	//
	cuda_device T dot(const vector1& v) const {
		return (this->x * v.x);
	}
	cuda_device friend T dot(const T& v1, const T& v2) {
		return (v1 * v2);
	}
	cuda_device friend T length(const T& v) {
		return sqrt_T(v.dot(v));
	}
	cuda_device friend T fast_length(const T& v) {
		return fast_sqrt_T(v.dot(v));
	}
	cuda_device friend vector1 normalize(const T& v) {
		return v * rsqrt_T(v.dot(v));
	}
	cuda_device friend vector1 fast_normalize(const T& v) {
		return v * fast_rsqrt_T(v.dot(v));
	}
	cuda_device friend T distance(const T& v1, const T& v2) {
		return length(v1 - v2);
	}
	cuda_device friend T fast_distance(const T& v1, const T& v2) {
		return fast_length(v1 - v2);
	}
	
	// generic: -->
	cuda_device const T& operator[](const uint index) const {
		return ((T*)this)[index];
	}
	cuda_device T& operator[](const uint index) {
		return ((T*)this)[index];
	}
};

// note: cuda doesn't align vec1 types
typedef cuda_device_builtin vector1<char> char1;
typedef cuda_device_builtin vector1<uchar> uchar1;
typedef cuda_device_builtin vector1<short> short1;
typedef cuda_device_builtin vector1<ushort> ushort1;
typedef cuda_device_builtin vector1<int> int1;
typedef cuda_device_builtin vector1<uint> uint1;
typedef cuda_device_builtin vector1<long> long1;
typedef cuda_device_builtin vector1<ulong> ulong1;
typedef cuda_device_builtin vector1<float> float1;
typedef cuda_device_builtin vector1<double> double1;
typedef cuda_device_builtin struct vector1<ulong> ulonglong1;

// additional cuda types
typedef cuda_device_builtin vector1<long> longlong1;
typedef cuda_device_builtin vector1<ulong> ulonglong1;

#endif
#endif
