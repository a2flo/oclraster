
#ifndef __FLOOR_CUDA_VECTOR2_H__
#define __FLOOR_CUDA_VECTOR2_H__

#if defined(__cplusplus)

#undef CUDA_VECTOR_WIDTH
#define CUDA_VECTOR_WIDTH 2
#include "floor_cuda_vector_ops.hpp"

template <typename T, uint alignment> struct cuda_device_builtin __attribute__((aligned(alignment))) vector2 {
	T x;
	T y;
	
	// the empty default constructor must not initialize any member variables,
	// so that it can be used for shared/local memory variables!
	cuda_device vector2() {}
	cuda_device vector2(const vector2& vec) : x(vec.x), y(vec.y) {}
	cuda_device vector2(const T& vx, const T& vy) : x(vx), y(vy) {}
	cuda_device vector2(const T& val) : x(val), y(val) {}
	
	cuda_device vector2& operator=(const vector2& vec) {
		this->x = vec.x;
		this->y = vec.y;
		return *this;
	}
	cuda_device vector2& operator=(const T& val) {
		this->x = val;
		this->y = val;
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
	cuda_device T dot(const vector2& v) const {
		return (this->x * v.x + this->y * v.y);
	}
	cuda_device friend T dot(const vector2& v1, const vector2& v2) {
		return v1.dot(v2);
	}
	cuda_device friend T length(const vector2& v) {
		return sqrt_T(v.dot(v));
	}
	cuda_device friend T fast_length(const vector2& v) {
		return fast_sqrt_T(v.dot(v));
	}
	cuda_device friend vector2 normalize(const vector2& v) {
		return v * rsqrt_T(v.dot(v));
	}
	cuda_device friend vector2 fast_normalize(const vector2& v) {
		return v * fast_rsqrt_T(v.dot(v));
	}
	cuda_device friend T distance(const vector2& v1, const vector2& v2) {
		return length(v1 - v2);
	}
	cuda_device friend T fast_distance(const vector2& v1, const vector2& v2) {
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

typedef vector2<char, 2> char2;
typedef vector2<uchar, 2> uchar2;
typedef vector2<short, 4> short2;
typedef vector2<ushort, 4> ushort2;
typedef vector2<int, 8> int2;
typedef vector2<uint, 8> uint2;
typedef vector2<long, 16> long2;
typedef vector2<ulong, 16> ulong2;
typedef vector2<float, 8> float2;
typedef vector2<double, 16> double2;

// additional cuda types
typedef vector2<long, 16> longlong2;
typedef vector2<ulong, 16> ulonglong2;

#endif
#endif
