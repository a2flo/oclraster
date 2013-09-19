
#ifndef __FLOOR_CUDA_VECTOR3_H__
#define __FLOOR_CUDA_VECTOR3_H__

#if defined(__cplusplus)

#undef CUDA_VECTOR_WIDTH
#define CUDA_VECTOR_WIDTH 3
#include "floor_cuda_vector_ops.hpp"

template <typename T> struct cuda_device_builtin vector3 {
	T x;
	T y;
	T z;
	
	// the empty default constructor must not initialize any member variables,
	// so that it can be used for shared/local memory variables!
	cuda_device vector3() {}
	cuda_device vector3(const vector3& vec) : x(vec.x), y(vec.y), z(vec.z) {}
	cuda_device vector3(const T& vx, const T& vy, const T& vz) : x(vx), y(vy), z(vz) {}
	cuda_device vector3(const T& val) : x(val), y(val), z(val) {}
	
	// from vec2
	template <uint alignment> cuda_device vector3(const vector2<T, alignment>& vec2, const T& val) : x(vec2.x), y(vec2.y), z(val) {}
	template <uint alignment> cuda_device vector3(const T& val, const vector2<T, alignment>& vec2) : x(val), y(vec2.x), z(vec2.y) {}
	
	
	cuda_device vector3& operator=(const vector3& vec) {
		this->x = vec.x;
		this->y = vec.y;
		this->z = vec.z;
		return *this;
	}
	cuda_device vector3& operator=(const T& val) {
		this->x = val;
		this->y = val;
		this->z = val;
		return *this;
	}
	
	// from vec2
	template <uint alignment> cuda_device vector3& operator=(const vector2<T, alignment>& vec) {
		this->x = vec.x;
		this->y = vec.y;
		// keep this->z
		return *this;
	}
	
	//
	cuda_device vector3 cross(const vector3& v) const {
		return vector3(this->y * v.z - this->z * v.y,
					   this->z * v.x - this->x * v.z,
					   this->x * v.y - this->y * v.x);
	}
	cuda_device friend vector3 cross(const vector3& v1, const vector3& v2) {
		return v1.cross(v2);
	}
	
	cuda_device T dot(const vector3& v) const {
		return (this->x * v.x + this->y * v.y + this->z * v.z);
	}
	cuda_device friend T dot(const vector3& v1, const vector3& v2) {
		return v1.dot(v2);
	}
	cuda_device friend T length(const vector3& v) {
		return sqrt_T(v.dot(v));
	}
	cuda_device friend T fast_length(const vector3& v) {
		return fast_sqrt_T(v.dot(v));
	}
	cuda_device friend vector3 normalize(const vector3& v) {
		return v * rsqrt_T(v.dot(v));
	}
	cuda_device friend vector3 fast_normalize(const vector3& v) {
		return v * fast_rsqrt_T(v.dot(v));
	}
	cuda_device friend T distance(const vector3& v1, const vector3& v2) {
		return length(v1 - v2);
	}
	cuda_device friend T fast_distance(const vector3& v1, const vector3& v2) {
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

// note: cuda doesn't align vec3 types
typedef vector3<char> char3;
typedef vector3<uchar> uchar3;
typedef vector3<short> short3;
typedef vector3<ushort> ushort3;
typedef vector3<int> int3;
typedef vector3<uint> uint3;
typedef vector3<long> long3;
typedef vector3<ulong> ulong3;
typedef vector3<float> float3;
typedef vector3<double> double3;

// additional cuda types
typedef vector3<long> longlong3;
typedef vector3<ulong> ulonglong3;

template <typename T> struct cuda_device_builtin dim3_T {
	T x;
	T y;
	T z;
	
	cuda_device dim3_T() : x((T)1), y((T)1), z((T)1) {}
	cuda_device dim3_T(const dim3_T& vec) : x(vec.x), y(vec.y), z(vec.z) {}
	cuda_device dim3_T(const vector3<T>& vec) : x(vec.x), y(vec.y), z(vec.z) {}
	cuda_device dim3_T(const T& vx, const T& vy, const T& vz) : x(vx), y(vy), z(vz) {}
	
};
typedef dim3_T<uint> dim3;

#endif
#endif
