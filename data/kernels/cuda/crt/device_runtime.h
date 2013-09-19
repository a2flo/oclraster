
#ifndef __OCLRASTER_CUDA_DEVICE_RUNTIME_H__
#define __OCLRASTER_CUDA_DEVICE_RUNTIME_H__

typedef struct _Z7vector3IjE uint3;
typedef struct _Z6dim3_TIjE dim3;

#if defined(__CUDABE__)
#include "oclr_cuda_vector_lib.hpp"

#define __SURFACE_FUNCTIONS_H__
#define __TEXTURE_FETCH_FUNCTIONS_H__
#endif

#include "/usr/local/cuda/include/crt/device_runtime.h"

#endif
