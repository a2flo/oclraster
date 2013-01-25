
#ifndef __OCLRASTER_CUDACL_H__
#define __OCLRASTER_CUDACL_H__

// defines
#define OCLRASTER_FUNC inline __device__
#define OCLRASTER_CUDA_CL 1

// types
typedef unsigned int uint;
typedef struct {
	float4 lo;
	float4 hi;
} float8;
typedef struct {
	float8 lo;
	float8 hi;
} float16;

// global/local work item/dim functions
OCLRASTER_FUNC uint get_work_dim() {
	if(gridDim.y == 0) return 1;
	if(gridDim.z == 0) return 2;
	return 3;
}

OCLRASTER_FUNC size_t get_global_size(uint dimindx) {
	switch(dimindx) {
		case 0: return gridDim.x;
		case 1: return gridDim.y;
		case 2: return gridDim.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_global_id(uint dimindx) {
	switch(dimindx) {
		case 0: return blockDim.x * blockIdx.x + threadIdx.x;
		case 1: return blockDim.y * blockIdx.y + threadIdx.y;
		case 2: return blockDim.z * blockIdx.z + threadIdx.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_local_size(uint dimindx) {
	switch(dimindx) {
		case 0: return blockDim.x;
		case 1: return blockDim.y;
		case 2: return blockDim.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_local_id(uint dimindx) {
	switch(dimindx) {
		case 0: return threadIdx.x;
		case 1: return threadIdx.y;
		case 2: return threadIdx.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_num_groups(uint dimindx) {
	switch(dimindx) {
		case 0: return gridDim.x / blockDim.x;
		case 1: return gridDim.y / blockDim.y;
		case 2: return gridDim.z / blockDim.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_group_id(uint dimindx) {
	switch(dimindx) {
		case 0: return blockIdx.x;
		case 1: return blockIdx.y;
		case 2: return blockIdx.z;
		default: break;
	}
	return 0;
}

OCLRASTER_FUNC size_t get_global_offset(uint dimindx) {
	return 0; // not required/supported by opencl
}

// barriers/fences/sync
// TODO: __threadfence?
#define barrier(X) __syncthreads();
#define mem_fence(X) __syncthreads();
#define read_mem_fence(X) __syncthreads();
#define write_mem_fence(X) __syncthreads();

// helper functions
OCLRASTER_FUNC float2 sin(float2 vec) {
	return make_float2(sin(vec.x), sin(vec.y));
}
OCLRASTER_FUNC float3 sin(float3 vec) {
	return make_float3(sin(vec.x), sin(vec.y), sin(vec.z));
}
OCLRASTER_FUNC float4 sin(float4 vec) {
	return make_float4(sin(vec.x), sin(vec.y), sin(vec.z), sin(vec.w));
}
OCLRASTER_FUNC float2 cos(float2 vec) {
	return make_float2(cos(vec.x), cos(vec.y));
}
OCLRASTER_FUNC float3 cos(float3 vec) {
	return make_float3(cos(vec.x), cos(vec.y), cos(vec.z));
}
OCLRASTER_FUNC float4 cos(float4 vec) {
	return make_float4(cos(vec.x), cos(vec.y), cos(vec.z), cos(vec.w));
}
OCLRASTER_FUNC float2 tan(float2 vec) {
	return make_float2(tan(vec.x), tan(vec.y));
}
OCLRASTER_FUNC float3 tan(float3 vec) {
	return make_float3(tan(vec.x), tan(vec.y), tan(vec.z));
}
OCLRASTER_FUNC float4 tan(float4 vec) {
	return make_float4(tan(vec.x), tan(vec.y), tan(vec.z), tan(vec.w));
}

template <typename T> OCLRASTER_FUNC float distance(T vec_0, T vec_1) {
	const T diff = vec_0 - vec_1;
	return sqrtf(dot(diff, diff));
}
template <typename T> OCLRASTER_FUNC float fast_distance(T vec_0, T vec_1) {
	const T diff = vec_0 - vec_1;
	return __fsqrt_rn(dot(diff, diff));
}
template <typename T> OCLRASTER_FUNC float fast_length(T vec) {
	return __fsqrt_rn(dot(vec, vec));
}
template <typename T> OCLRASTER_FUNC T fast_normalize(T vec) {
	return normalize(vec);
}

//
OCLRASTER_FUNC float2 make_float2(float4 vec) {
	return make_float2(vec.x, vec.y);
}

// redundant make_float*
OCLRASTER_FUNC float2 make_float2(float2 vec) {
	return vec;
}
OCLRASTER_FUNC float3 make_float3(float3 vec) {
	return vec;
}
OCLRASTER_FUNC float4 make_float4(float4 vec) {
	return vec;
}

#endif
