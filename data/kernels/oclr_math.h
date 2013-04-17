
#ifndef __OCLRASTER_MATH_H__
#define __OCLRASTER_MATH_H__

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

#include "oclr_global.h"

//
#define PI 3.1415926535897932384626433832795
#define _180DIVPI 57.295779513082322
#define _PIDIV180 0.01745329251994
#define _PIDIV360 0.00872664625997
#define RAD2DEG(rad) (rad * _180DIVPI)
#define DEG2RAD(deg) (deg * _PIDIV180)
#define EPSILON 0.00001f

//
OCLRASTER_FUNC float det3(const float3 v0, const float3 v1, const float3 v2) {
	return dot(v0, cross(v1, v2));
}
OCLRASTER_FUNC float det4(const float4 v0, const float4 v1, const float4 v2, const float4 v3) {
	float d = v3.w * det3(v0.xyz, v1.xyz, v2.xyz);
	d -= v2.w * det3(v0.xyz, v1.xyz, v3.xyz);
	d += v1.w * det3(v0.xyz, v2.xyz, v3.xyz);
	d -= v0.w * det3(v1.xyz, v2.xyz, v3.xyz);
	return d;
}

OCLRASTER_FUNC float3 reflect(const float3 I, const float3 N) {
	return I - 2.0f * dot(N, I) * N;
}

//
#define interpolate(v0, v1, v2, gad) (mad(v0, gad.x, mad(v1, gad.y, v2 * gad.z)))
#define linear_blend(v0, v1, interp) (mad(v1 - v0, interp, v0))

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
