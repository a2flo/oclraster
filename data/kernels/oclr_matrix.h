
#ifndef __OCLRASTER_MATRIX_H__
#define __OCLRASTER_MATRIX_H__

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

//
typedef struct {
	float2 m[2];
} mat2;
typedef struct {
	float3 m[3];
} mat3;
typedef struct {
	float4 m[4];
} mat4;


OCLRASTER_FUNC float3 mat3_mul_vec3(const mat3 mat, const float3 vec) {
	return (vec.x * mat.m[0]) + (vec.y * mat.m[1]) + (vec.z * mat.m[2]);
}
OCLRASTER_FUNC float4 mat3_mul_vec4(const mat3 mat, const float4 vec) {
	const float3 vec3 = (vec.x * mat.m[0]) + (vec.y * mat.m[1]) + (vec.z * mat.m[2]);
	return (float4)(vec3.x, vec3.y, vec3.z, vec.w);
}
OCLRASTER_FUNC float3 mat4_mul_vec3(const mat4 mat, const float3 vec) {
	return (vec.x * mat.m[0].xyz) + (vec.y * mat.m[1].xyz) + (vec.z * mat.m[2].xyz);
}
OCLRASTER_FUNC float4 mat4_mul_vec4(const mat4 mat, const float4 vec) {
	return (vec.x * mat.m[0]) + (vec.y * mat.m[1]) + (vec.z * mat.m[2]) + (vec.w * mat.m[3]);
}

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
