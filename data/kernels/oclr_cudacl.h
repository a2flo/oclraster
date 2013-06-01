
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

typedef unsigned char uchar;
typedef unsigned long long int ulong;

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

OCLRASTER_FUNC int signbit(const float& val) {
	return __int2float_rn(__signbitf(val));
}
OCLRASTER_FUNC int2 signbit(const float2& vec) {
	return make_int2(__signbitf(vec.x), __signbitf(vec.y));
}
OCLRASTER_FUNC int3 signbit(const float3& vec) {
	return make_int3(__signbitf(vec.x), __signbitf(vec.y), __signbitf(vec.z));
}
OCLRASTER_FUNC int4 signbit(const float4& vec) {
	return make_int4(__signbitf(vec.x), __signbitf(vec.y), __signbitf(vec.z), __signbitf(vec.w));
}

OCLRASTER_FUNC int any(const int2& val) {
	return ((val.x & 1) || (val.y & 1));
}
OCLRASTER_FUNC int any(const int3& val) {
	return ((val.x & 1) || (val.y & 1) || (val.z & 1));
}
OCLRASTER_FUNC int any(const int4& val) {
	return ((val.x & 1) || (val.y & 1) || (val.z & 1) || (val.w & 1));
}

OCLRASTER_FUNC int all(const int2& val) {
	return ((val.x & 1) && (val.y & 1));
}
OCLRASTER_FUNC int all(const int3& val) {
	return ((val.x & 1) && (val.y & 1) && (val.z & 1));
}
OCLRASTER_FUNC int all(const int4& val) {
	return ((val.x & 1) && (val.y & 1) && (val.z & 1) && (val.w & 1));
}

// for mad instructions: let the compiler decide what to do
OCLRASTER_FUNC float mad(const float a, const float b, const float c) {
	return a * b + c;
}
OCLRASTER_FUNC float2 mad(const float2 a, const float2 b, const float2 c) {
	return make_float2(a.x * b.x + c.x, a.y * b.y + c.y);
}
OCLRASTER_FUNC float3 mad(const float3 a, const float3 b, const float3 c) {
	return make_float3(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z);
}
OCLRASTER_FUNC float4 mad(const float4 a, const float4 b, const float4 c) {
	return make_float4(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z, a.w * b.w + c.w);
}

// for explicit fma instructions: use built-ins
// --single float fma is already defined
OCLRASTER_FUNC float2 fma(const float2 a, const float2 b, const float2 c) {
	return make_float2(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y));
}
OCLRASTER_FUNC float3 fma(const float3 a, const float3 b, const float3 c) {
	return make_float3(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y), __fmaf_rz(a.z, b.z, c.z));
}
OCLRASTER_FUNC float4 fma(const float4 a, const float4 b, const float4 c) {
	return make_float4(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y), __fmaf_rz(a.z, b.z, c.z), __fmaf_rz(a.w, b.w, c.w));
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

// atomics (note: cuda atomic inc/dec works differently -> use add/sub)
#define atomic_inc(a) atomicAdd(a, 1)
#define atomic_dec(a) AtomicSub(a, 1)
#define atomic_add(a, v) AtomicAdd(a, v)
#define atomic_sub(a, v) AtomicSub(a, v)
#define atomic_xchg(a, v) AtomicExch(a, v)
#define atomic_cmpxchg(a, c, v) AtomicCAS(a, c, v)
#define atomic_min(a, v) AtomicMin(a, v)
#define atomic_max(a, v) AtomicMax(a, v)
#define atomic_and(a, v) AtomicAnd(a, v)
#define atomic_or(a, v) AtomicOr(a, v)
#define atomic_xor(a, v) AtomicXor(a, v)

// async function emulation
typedef int event_t;

// cuda doesn't care about address spaces (-> same code for global->local and local->global copy)
template <typename gentype>
OCLRASTER_FUNC event_t async_work_group_copy(gentype* dst,
											 const gentype* src,
											 const size_t num_gentypes,
											 event_t event) {
	//
	const size_t work_size = blockDim.x * blockDim.y * blockDim.z;
	const size_t work_per_item = num_gentypes / work_size; // min amount for all items, remaining work will be done individually at the end
	const size_t remaining = num_gentypes - (work_per_item * work_size);
	const size_t idx = threadIdx.x + (threadIdx.y * blockDim.x) + (threadIdx.z * (blockDim.x * blockDim.y));
	if(idx >= num_gentypes) return 0; // not enough work
	
	// copy data using all work-items (interleaved)
	gentype* dst_ptr = dst + idx;
	const gentype* src_ptr = src + idx;
	for(size_t i = 0; i < work_per_item; i++) {
		*dst_ptr = *src_ptr;
		dst_ptr += work_size;
		src_ptr += work_size;
	}
	
	// copy remaining items
	if(idx < remaining) {
		*dst_ptr = *src_ptr;
	}
	
	return 0;
}

OCLRASTER_FUNC void wait_group_events(int num_events, event_t* event_list) {
	// basically a no-op with sync
	// all work is done previously, synchronously across all items, but still need to sync
	__syncthreads();
}

// conversion functions
// note that round-to-zero (rz/rtz) is the default for conversions to integer
OCLRASTER_FUNC unsigned int convert_uint(const float val) { return __float2uint_rz(val); }
OCLRASTER_FUNC uint4 convert_uint4(const float4 val) {
	return make_uint4(__float2uint_rz(val.x), __float2uint_rz(val.y), __float2uint_rz(val.z), __float2uint_rz(val.w));
}
OCLRASTER_FUNC unsigned char convert_uchar(const float val) { return (unsigned char)__float2uint_rz(val); }
OCLRASTER_FUNC unsigned char convert_uchar_sat(const float val) { return (unsigned char)clamp(__float2uint_rz(val), 0u, 255u); }
OCLRASTER_FUNC float convert_float(const unsigned int val) { return __uint2float_rz(val); }

// vload* / vstore*
template <typename gentype, typename gentypen, size_t N> OCLRASTER_FUNC void vstoren(const gentypen& data, const size_t& offset, gentype* ptr) {
	const gentype* data_ptr = (const gentype*)&data;
	gentype* write_ptr = ptr + (offset * N);
#pragma unroll
	for(int i = 0; i < N; i++) {
		*write_ptr++ = *data_ptr++;
	}
}
template <typename gentype, typename gentypen> OCLRASTER_FUNC void vstore2(const gentypen& data, const size_t& offset, gentype* ptr) {
	vstoren<gentype, gentypen, 2>(data, offset, ptr);
}
template <typename gentype, typename gentypen> OCLRASTER_FUNC void vstore3(const gentypen& data, const size_t& offset, gentype* ptr) {
	vstoren<gentype, gentypen, 3>(data, offset, ptr);
}
template <typename gentype, typename gentypen> OCLRASTER_FUNC void vstore4(const gentypen& data, const size_t& offset, gentype* ptr) {
	vstoren<gentype, gentypen, 4>(data, offset, ptr);
}
template <typename gentype, typename gentypen> OCLRASTER_FUNC void vstore8(const gentypen& data, const size_t& offset, gentype* ptr) {
	vstoren<gentype, gentypen, 8>(data, offset, ptr);
}
template <typename gentype, typename gentypen> OCLRASTER_FUNC void vstore16(const gentypen& data, const size_t& offset, gentype* ptr) {
	vstoren<gentype, gentypen, 16>(data, offset, ptr);
}

#endif
