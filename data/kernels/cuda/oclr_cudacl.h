
#ifndef __OCLRASTER_CUDACL_H__
#define __OCLRASTER_CUDACL_H__

// note: the oclr_cuda_base.h header contains mostly cuda related functions or wrappers,
// while this header implements most of the opencl built-in functions
// exceptions:
//  * type conversions and reinterpretations (oclr_cuda_conversion.h, already included by oclr_cuda_base.h)
//  * clamp (implemented in oclr_cuda_base.h, as it's required earlier)
#include "oclr_cuda_base.h"

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

// vector helper functions
template<size_t component_index_0,
		 class vec_type,
		 class ret_vec_type = typename vector_mapping<vec_type, 1>::type,
		 class single_type = typename vector_mapping<vec_type, 1>::type>
OCLRASTER_FUNC ret_vec_type get_vector_components_1(const vec_type& vec) {
	return ((const single_type*)&vec)[component_index_0];
}

template<size_t component_index_0, size_t component_index_1,
		 class vec_type,
		 class ret_vec_type = typename vector_mapping<vec_type, 2>::type,
		 class single_type = typename vector_mapping<vec_type, 1>::type>
OCLRASTER_FUNC ret_vec_type get_vector_components_2(const vec_type& vec) {
	ret_vec_type ret;
	ret.x = ((const single_type*)&vec)[component_index_0];
	ret.y = ((const single_type*)&vec)[component_index_1];
	return ret;
}

template<size_t component_index_0, size_t component_index_1, size_t component_index_2,
		 class vec_type,
		 class ret_vec_type = typename vector_mapping<vec_type, 3>::type,
		 class single_type = typename vector_mapping<vec_type, 1>::type>
OCLRASTER_FUNC ret_vec_type get_vector_components_3(const vec_type& vec) {
	ret_vec_type ret;
	ret.x = ((const single_type*)&vec)[component_index_0];
	ret.y = ((const single_type*)&vec)[component_index_1];
	ret.z = ((const single_type*)&vec)[component_index_2];
	return ret;
}

template<size_t component_index_0, size_t component_index_1, size_t component_index_2, size_t component_index_3,
		 class vec_type,
		 class ret_vec_type = typename vector_mapping<vec_type, 4>::type,
		 class single_type = typename vector_mapping<vec_type, 1>::type>
OCLRASTER_FUNC ret_vec_type get_vector_components_4(const vec_type& vec) {
	ret_vec_type ret;
	ret.x = ((const single_type*)&vec)[component_index_0];
	ret.y = ((const single_type*)&vec)[component_index_1];
	ret.z = ((const single_type*)&vec)[component_index_2];
	ret.w = ((const single_type*)&vec)[component_index_3];
	return ret;
}

// TODO: !
template<class dst_vec_type, class single_type, class src_vec_type,
		 size_t component_index_0, size_t component_index_1, size_t component_index_2, size_t component_index_3>
OCLRASTER_FUNC void set_vector_components_4(const dst_vec_type& dst, const src_vec_type& src) {
	ret_vec_type ret;
	ret.x = ((const single_type*)&vec)[component_index_0];
	ret.y = ((const single_type*)&vec)[component_index_1];
	ret.z = ((const single_type*)&vec)[component_index_2];
	ret.w = ((const single_type*)&vec)[component_index_3];
	return ret;
}

// barriers/fences/sync
// TODO: __threadfence?
#define barrier(X) __syncthreads();
#define mem_fence(X) __syncthreads();
#define read_mem_fence(X) __syncthreads();
#define write_mem_fence(X) __syncthreads();

// helper functions
OCLRASTER_FUNC int signbit(const float& val) {
	return __int2float_rn(__signbitf(val));
}
OCLRASTER_FUNC int2 signbit(const float2& vec) {
	return int2(__signbitf(vec.x), __signbitf(vec.y));
}
OCLRASTER_FUNC int3 signbit(const float3& vec) {
	return int3(__signbitf(vec.x), __signbitf(vec.y), __signbitf(vec.z));
}
OCLRASTER_FUNC int4 signbit(const float4& vec) {
	return int4(__signbitf(vec.x), __signbitf(vec.y), __signbitf(vec.z), __signbitf(vec.w));
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

template<class gentype,
		 typename enable_if<is_floating_point<gentype>::value, int>::type = 0,
		 class single_type = typename vector_mapping<gentype, 1>::type>
OCLRASTER_FUNC gentype smoothstep(const gentype edge0, const gentype edge1, const gentype x) {
	const gentype t = clamp((x - edge0) / (edge1 - edge0), (single_type)0, (single_type)1);
	return t * t * (((single_type)3) - ((single_type)2) * t);
}

// for mad instructions: let the compiler decide what to do
OCLRASTER_FUNC float mad(const float a, const float b, const float c) {
	return a * b + c;
}
OCLRASTER_FUNC float2 mad(const float2 a, const float2 b, const float2 c) {
	return float2(a.x * b.x + c.x, a.y * b.y + c.y);
}
OCLRASTER_FUNC float3 mad(const float3 a, const float3 b, const float3 c) {
	return float3(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z);
}
OCLRASTER_FUNC float4 mad(const float4 a, const float4 b, const float4 c) {
	return float4(a.x * b.x + c.x, a.y * b.y + c.y, a.z * b.z + c.z, a.w * b.w + c.w);
}
OCLRASTER_FUNC float2 mad(const float2 a, const float b, const float2 c) {
	return float2(a.x * b + c.x, a.y * b + c.y);
}
OCLRASTER_FUNC float3 mad(const float3 a, const float b, const float3 c) {
	return float3(a.x * b + c.x, a.y * b + c.y, a.z * b + c.z);
}
OCLRASTER_FUNC float4 mad(const float4 a, const float b, const float4 c) {
	return float4(a.x * b + c.x, a.y * b + c.y, a.z * b + c.z, a.w * b + c.w);
}
OCLRASTER_FUNC float2 mad(const float2 a, const float2 b, const float c) {
	return float2(a.x * b.x + c, a.y * b.y + c);
}
OCLRASTER_FUNC float3 mad(const float3 a, const float3 b, const float c) {
	return float3(a.x * b.x + c, a.y * b.y + c, a.z * b.z + c);
}
OCLRASTER_FUNC float4 mad(const float4 a, const float4 b, const float c) {
	return float4(a.x * b.x + c, a.y * b.y + c, a.z * b.z + c, a.w * b.w + c);
}

// for explicit fma instructions: use built-ins
// --single float fma is already defined
OCLRASTER_FUNC float2 fma(const float2 a, const float2 b, const float2 c) {
	return float2(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y));
}
OCLRASTER_FUNC float3 fma(const float3 a, const float3 b, const float3 c) {
	return float3(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y), __fmaf_rz(a.z, b.z, c.z));
}
OCLRASTER_FUNC float4 fma(const float4 a, const float4 b, const float4 c) {
	return float4(__fmaf_rz(a.x, b.x, c.x), __fmaf_rz(a.y, b.y, c.y), __fmaf_rz(a.z, b.z, c.z), __fmaf_rz(a.w, b.w, c.w));
}

// atomics (note: cuda atomic inc/dec works differently -> use add/sub)
#define atomic_inc(a) atomicAdd(a, 1)
#define atomic_dec(a) atomicSub(a, 1)
#define atomic_add(a, v) atomicAdd(a, v)
#define atomic_sub(a, v) atomicSub(a, v)
#define atomic_xchg(a, v) atomicExch(a, v)
#define atomic_cmpxchg(a, c, v) atomicCAS(a, c, v)
#define atomic_min(a, v) atomicMin(a, v)
#define atomic_max(a, v) atomicMax(a, v)
#define atomic_and(a, v) atomicAnd(a, v)
#define atomic_or(a, v) atomicOr(a, v)
#define atomic_xor(a, v) atomicXor(a, v)

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

// half (gentype: half, gentypen: floatn)
template <typename gentypen, size_t N> OCLRASTER_FUNC void vstore_halfn(const gentypen& data, const size_t& offset, half* ptr) {
	const float* data_ptr = (const float*)&data;
	half* write_ptr = ptr + (offset * N);
#pragma unroll
	for(int i = 0; i < N; i++) {
		*write_ptr++ = __float2half_rn(*data_ptr++);
	}
}
OCLRASTER_FUNC void vstore_half(const float& data, const size_t& offset, half* ptr) {
	vstore_halfn<float, 1>(data, offset, ptr);
}
OCLRASTER_FUNC void vstore_half2(const float2& data, const size_t& offset, half* ptr) {
	vstore_halfn<float2, 2>(data, offset, ptr);
}
OCLRASTER_FUNC void vstore_half3(const float3& data, const size_t& offset, half* ptr) {
	vstore_halfn<float3, 3>(data, offset, ptr);
}
OCLRASTER_FUNC void vstore_half4(const float4& data, const size_t& offset, half* ptr) {
	vstore_halfn<float4, 4>(data, offset, ptr);
}
OCLRASTER_FUNC void vstore_half8(const float8& data, const size_t& offset, half* ptr) {
	vstore_halfn<float8, 8>(data, offset, ptr);
}
OCLRASTER_FUNC void vstore_half16(const float16& data, const size_t& offset, half* ptr) {
	vstore_halfn<float16, 16>(data, offset, ptr);
}

// sampler
typedef unsigned int sampler_t;
enum {
	// addressing mode
	CLK_ADDRESS_NONE				= (1u << 0u),
	CLK_ADDRESS_CLAMP				= (1u << 1u),
	CLK_ADDRESS_CLAMP_TO_EDGE		= (1u << 2u),
	CLK_ADDRESS_REPEAT				= (1u << 3u),
	CLK_ADDRESS_MIRRORED_REPEAT		= (1u << 4u),
	
	// normalized coords
	CLK_NORMALIZED_COORDS_FALSE		= (1u << 5u),
	CLK_NORMALIZED_COORDS_TRUE		= (1u << 6u),
	
	// filter mode
	CLK_FILTER_NEAREST				= (1u << 7u),
	CLK_FILTER_LINEAR				= (1u << 8u),
};

// channel data type
enum {
	CLK_SNORM_INT8,
	CLK_SNORM_INT16,
	CLK_UNORM_INT8,
	CLK_UNORM_INT16,
	CLK_UNORM_SHORT_565,
	CLK_UNORM_SHORT_555,
	CLK_UNORM_SHORT_101010,
	CLK_SIGNED_INT8,
	CLK_SIGNED_INT16,
	CLK_SIGNED_INT32,
	CLK_UNSIGNED_INT8,
	CLK_UNSIGNED_INT16,
	CLK_UNSIGNED_INT32,
	CLK_HALF_FLOAT,
	CLK_FLOAT,
};

// image channel order
enum {
	CLK_A,
	CLK_R,
	CLK_Rx,
	CLK_RG,
	CLK_RGx,
	CLK_RA,
	CLK_RGB,
	CLK_RGBx,
	CLK_RGBA,
	CLK_ARGB,
	CLK_BGRA,
	CLK_INTENSITY,
	CLK_LUMINANCE,
};

// native image functions (no-ops for now)
typedef texture<uchar, cudaTextureType2D, cudaReadModeElementType> image2d_t;
template <class coord_type> OCLRASTER_FUNC float4 read_imagef(image2d_t img, const sampler_t& sampler, const coord_type& coord) {
	return float4(0.0f);
};
template <class coord_type> OCLRASTER_FUNC int4 read_imagei(image2d_t img, const sampler_t& sampler, const coord_type& coord) {
	return int4(0);
};
template <class coord_type> OCLRASTER_FUNC uint4 read_imageui(image2d_t img, const sampler_t& sampler, const coord_type& coord) {
	return uint4(0u);
};
void write_imagef(image2d_t img, const int2& coord, const float4& color) {}
void write_imagei(image2d_t img, const int2& coord, const int4& color) {}
void write_imageui(image2d_t img, const int2& coord, const uint4& color) {}

#endif
