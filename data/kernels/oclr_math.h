
#ifndef __OCLRASTER_MATH_H__
#define __OCLRASTER_MATH_H__

// ignore all "unused function" warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused"
#endif

#include "oclr_global.h"

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
#define interpolate(v0, v1, v2, gad) ((v0 * gad.x) + (v1 * gad.y) + (v2 * gad.z))

//
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
