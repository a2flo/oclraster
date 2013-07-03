
#ifndef __OCLRASTER_CUDA_VECTOR4_H__
#define __OCLRASTER_CUDA_VECTOR4_H__

#if defined(__cplusplus)

#undef CUDA_VECTOR_WIDTH
#define CUDA_VECTOR_WIDTH 4
#include "oclr_cuda_vector_ops.h"

template <typename T, uint alignment> struct cuda_device_builtin __attribute__((aligned(alignment))) vector4 {
	T x;
	T y;
	T z;
	T w;
	
	// the empty default constructor must not initialize any member variables,
	// so that it can be used for shared/local memory variables!
	cuda_device vector4() {}
	cuda_device vector4(const vector4& vec) : x(vec.x), y(vec.y), z(vec.z), w(vec.w) {}
	cuda_device vector4(const T& vx, const T& vy, const T& vz, const T& vw) : x(vx), y(vy), z(vz), w(vw) {}
	cuda_device vector4(const T& val) : x(val), y(val), z(val), w(val) {}
	
	// from vec2
	template <uint alignment_2> cuda_device vector4(const vector2<T, alignment_2>& vec2, const T& val_1, const T& val_2) : x(vec2.x), y(vec2.y), z(val_1), w(val_2) {}
	template <uint alignment_2> cuda_device vector4(const T& val_1, const vector2<T, alignment_2>& vec2, const T& val_2) : x(val_1), y(vec2.x), z(vec2.y), w(val_2) {}
	template <uint alignment_2> cuda_device vector4(const T& val_1, const T& val_2, const vector2<T, alignment_2>& vec2) : x(val_1), y(val_2), z(vec2.x), w(vec2.y) {}
	template <uint alignment_2> cuda_device vector4(const vector2<T, alignment_2>& vec2_1, const vector2<T, alignment_2>& vec2_2) : x(vec2_1.x), y(vec2_1.y), z(vec2_2.x), w(vec2_2.y) {}
	
	// from vec3
	cuda_device vector4(const vector3<T>& vec3, const T& val) : x(vec3.x), y(vec3.y), z(vec3.z), w(val) {}
	cuda_device vector4(const T& val, const vector3<T>& vec3) : x(val), y(vec3.x), z(vec3.y), w(vec3.z) {}
	
	cuda_device vector4& operator=(const vector4& vec) {
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
		this->w = vec.w;
		return *this;
	}
	cuda_device vector4& operator=(const T& val) {
		this->x = val;
		this->y = val;
		this->z = val;
		this->w = val;
		return *this;
	}
	
	// from vec2
	template <uint alignment_2> cuda_device vector4& operator=(const vector2<T, alignment_2>& vec) {
		this->x = vec.x;
		this->y = vec.y;
		// keep this->z and this->w
		return *this;
	}
	
	// from vec3
	cuda_device vector4& operator=(const vector3<T>& vec) {
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
		// keep this->w
		return *this;
	}
	
	//
	cuda_device vector4 cross(const vector4& v) const {
		return vector4(this->y * v.z - this->z * v.y,
					   this->z * v.x - this->x * v.z,
					   this->x * v.y - this->y * v.x,
					   (T)0);
	}
	cuda_device friend vector4 cross(const vector4& v1, const vector4& v2) {
		return v1.cross(v2);
	}
	
	cuda_device T dot(const vector4& v) const {
		return (this->x * v.x + this->y * v.y + this->z * v.z + this->w * v.w);
	}
	cuda_device friend T dot(const vector4& v1, const vector4& v2) {
		return v1.dot(v2);
	}
	cuda_device friend T length(const vector4& v) {
		return sqrt_T(v.dot(v));
	}
	cuda_device friend T fast_length(const vector4& v) {
		return fast_sqrt_T(v.dot(v));
	}
	cuda_device friend vector4 normalize(const vector4& v) {
		return v * rsqrt_T(v.dot(v));
	}
	cuda_device friend vector4 fast_normalize(const vector4& v) {
		return v * fast_rsqrt_T(v.dot(v));
	}
	cuda_device friend T distance(const vector4& v1, const vector4& v2) {
		return length(v1 - v2);
	}
	cuda_device friend T fast_distance(const vector4& v1, const vector4& v2) {
		return fast_length(v1 - v2);
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
	
	// generic: -->
	cuda_device const T& operator[](const uint index) const {
		return ((T*)this)[index];
	}
	cuda_device T& operator[](const uint index) {
		return ((T*)this)[index];
	}
	
};

//
typedef vector4<char, 4> char4;
typedef cuda_builtin_typedef vector4<uchar, 4> uchar4;
typedef vector4<short, 8> short4;
typedef vector4<ushort, 8> ushort4;
// always alignment of 16 from here on!
typedef vector4<int, 16> int4;
typedef vector4<uint, 16> uint4;
typedef vector4<long, 16> long4;
typedef vector4<ulong, 16> ulong4;
typedef vector4<float, 16> float4;
typedef vector4<double, 16> double4;

// additional cuda types
typedef vector4<long, 16> longlong4;
typedef vector4<ulong, 16> ulonglong4;

#endif
#endif
