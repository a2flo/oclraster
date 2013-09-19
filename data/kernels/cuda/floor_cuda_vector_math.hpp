
#ifndef __FLOOR_CUDA_VECTOR_MATH_H__
#define __FLOOR_CUDA_VECTOR_MATH_H__

//
#define CUDA_VEC_FUNC_ALIAS(type, cl_func, func) \
__device__ type cl_func(const type& val) { \
	return func(val); \
}
#define CUDA_VEC_VEC_FUNC_ALIAS(type, cl_func, func) \
__device__ type cl_func(const type& val1, const type& val2) { \
	return func(val1, val2); \
}

#define CUDA_VEC_FUNC_1(type, cl_func, func) \
__device__ type##1 cl_func(const type##1& vec) { \
	return type##1(func(vec.x)); \
}
#define CUDA_VEC_VEC_FUNC_1(type, cl_func, func) \
__device__ type##1 cl_func(const type##1& vec1, const type##1& vec2) { \
	return type##1(func(vec1.x, vec2.x)); \
}
#define CUDA_VEC_SINGLE_FUNC_1(type, cl_func, func) \
__device__ type##1 cl_func(const type##1& vec, const type& val) { \
	return type##1(func(vec.x, val)); \
}

#define CUDA_VEC_FUNC_2(type, cl_func, func) \
__device__ type##2 cl_func(const type##2& vec) { \
	return type##2(func(vec.x), func(vec.y)); \
}
#define CUDA_VEC_VEC_FUNC_2(type, cl_func, func) \
__device__ type##2 cl_func(const type##2& vec1, const type##2& vec2) { \
	return type##2(func(vec1.x, vec2.x), func(vec1.y, vec2.y)); \
}
#define CUDA_VEC_SINGLE_FUNC_2(type, cl_func, func) \
__device__ type##2 cl_func(const type##2& vec, const type& val) { \
	return type##2(func(vec.x, val), func(vec.y, val)); \
}

#define CUDA_VEC_FUNC_3(type, cl_func, func) \
__device__ type##3 cl_func(const type##3& vec) { \
	return type##3(func(vec.x), func(vec.y), func(vec.z)); \
}
#define CUDA_VEC_VEC_FUNC_3(type, cl_func, func) \
__device__ type##3 cl_func(const type##3& vec1, const type##3& vec2) { \
	return type##3(func(vec1.x, vec2.x), func(vec1.y, vec2.y), func(vec1.z, vec2.z)); \
}
#define CUDA_VEC_SINGLE_FUNC_3(type, cl_func, func) \
__device__ type##3 cl_func(const type##3& vec, const type& val) { \
	return type##3(func(vec.x, val), func(vec.y, val), func(vec.z, val)); \
}

#define CUDA_VEC_FUNC_4(type, cl_func, func) \
__device__ type##4 cl_func(const type##4& vec) { \
	return type##4(func(vec.x), func(vec.y), func(vec.z), func(vec.w)); \
}
#define CUDA_VEC_VEC_FUNC_4(type, cl_func, func) \
__device__ type##4 cl_func(const type##4& vec1, const type##4& vec2) { \
	return type##4(func(vec1.x, vec2.x), func(vec1.y, vec2.y), func(vec1.z, vec2.z), func(vec1.w, vec2.w)); \
}
#define CUDA_VEC_SINGLE_FUNC_4(type, cl_func, func) \
__device__ type##4 cl_func(const type##4& vec, const type& val) { \
	return type##4(func(vec.x, val), func(vec.y, val), func(vec.z, val), func(vec.w, val)); \
}

//
#define CUDA_VEC_MATH_FUNC(cl_func, cuda_float_func, cuda_double_func) \
CUDA_VEC_FUNC_ALIAS(float, cl_func, cuda_float_func) \
CUDA_VEC_FUNC_1(float, cl_func, cuda_float_func) \
CUDA_VEC_FUNC_1(double, cl_func, cuda_double_func) \
CUDA_VEC_FUNC_2(float, cl_func, cuda_float_func) \
CUDA_VEC_FUNC_2(double, cl_func, cuda_double_func) \
CUDA_VEC_FUNC_3(float, cl_func, cuda_float_func) \
CUDA_VEC_FUNC_3(double, cl_func, cuda_double_func) \
CUDA_VEC_FUNC_4(float, cl_func, cuda_float_func) \
CUDA_VEC_FUNC_4(double, cl_func, cuda_double_func)

#define CUDA_VEC_VEC_MATH_FUNC(cl_func, cuda_float_func, cuda_double_func) \
CUDA_VEC_VEC_FUNC_ALIAS(float, cl_func, cuda_float_func) \
CUDA_VEC_VEC_FUNC_1(float, cl_func, cuda_float_func) \
CUDA_VEC_VEC_FUNC_1(double, cl_func, cuda_double_func) \
CUDA_VEC_VEC_FUNC_2(float, cl_func, cuda_float_func) \
CUDA_VEC_VEC_FUNC_2(double, cl_func, cuda_double_func) \
CUDA_VEC_VEC_FUNC_3(float, cl_func, cuda_float_func) \
CUDA_VEC_VEC_FUNC_3(double, cl_func, cuda_double_func) \
CUDA_VEC_VEC_FUNC_4(float, cl_func, cuda_float_func) \
CUDA_VEC_VEC_FUNC_4(double, cl_func, cuda_double_func)

#define CUDA_VEC_SINGLE_MATH_FUNC(cl_func, cuda_float_func, cuda_double_func) \
CUDA_VEC_SINGLE_FUNC_1(float, cl_func, cuda_float_func) \
CUDA_VEC_SINGLE_FUNC_1(double, cl_func, cuda_double_func) \
CUDA_VEC_SINGLE_FUNC_2(float, cl_func, cuda_float_func) \
CUDA_VEC_SINGLE_FUNC_2(double, cl_func, cuda_double_func) \
CUDA_VEC_SINGLE_FUNC_3(float, cl_func, cuda_float_func) \
CUDA_VEC_SINGLE_FUNC_3(double, cl_func, cuda_double_func) \
CUDA_VEC_SINGLE_FUNC_4(float, cl_func, cuda_float_func) \
CUDA_VEC_SINGLE_FUNC_4(double, cl_func, cuda_double_func)

//
#define DECLARE_CUDA_MATH_FUNCS1(cl_func, cuda_float_func, cuda_double_func, func_macro_1) \
func_macro_1(cl_func, cuda_float_func, cuda_double_func)

#define DECLARE_CUDA_MATH_FUNCS2(cl_func, cuda_float_func, cuda_double_func, func_macro_1, func_macro_2) \
func_macro_1(cl_func, cuda_float_func, cuda_double_func) \
func_macro_2(cl_func, cuda_float_func, cuda_double_func)

#define DECLARE_CUDA_MATH_FUNCS3(cl_func, cuda_float_func, cuda_double_func, func_macro_1, func_macro_2, func_macro_3) \
func_macro_1(cl_func, cuda_float_func, cuda_double_func) \
func_macro_2(cl_func, cuda_float_func, cuda_double_func) \
func_macro_3(cl_func, cuda_float_func, cuda_double_func)

#define GET_DECLARE_CUDA_MATH_FUNCS_MACRO(arg0, arg1, arg2, macro_name, ...) macro_name

#define DECLARE_CUDA_MATH_FUNCS(cl_func, cuda_float_func, cuda_double_func, ...) \
GET_DECLARE_CUDA_MATH_FUNCS_MACRO(__VA_ARGS__, DECLARE_CUDA_MATH_FUNCS3, DECLARE_CUDA_MATH_FUNCS2, DECLARE_CUDA_MATH_FUNCS1,)(cl_func, cuda_float_func, cuda_double_func, __VA_ARGS__)

// TODO: rest of 6.12.2, 6.12.3, 6.12.4
// <cl func, float cuda func, double cuda func, opt args...>
#define __CL_CUDA_FP_MATH_FUNCS(F) \
F(fmax, fmaxf, fmax, CUDA_VEC_VEC_MATH_FUNC, CUDA_VEC_SINGLE_MATH_FUNC) \
F(fmin, fminf, fmin, CUDA_VEC_VEC_MATH_FUNC, CUDA_VEC_SINGLE_MATH_FUNC) \
F(floor, floorf, floor, CUDA_VEC_MATH_FUNC) \
F(ceil, ceilf, ceil, CUDA_VEC_MATH_FUNC) \
F(round, roundf, round, CUDA_VEC_MATH_FUNC) \
F(trunc, truncf, trunc, CUDA_VEC_MATH_FUNC) \
F(fabs, fabsf, fabs, CUDA_VEC_MATH_FUNC) \
F(fmod, fmodf, fmod, CUDA_VEC_VEC_MATH_FUNC, CUDA_VEC_SINGLE_MATH_FUNC) \
F(sin, sinf, sin, CUDA_VEC_MATH_FUNC) \
F(cos, cosf, cos, CUDA_VEC_MATH_FUNC) \
F(tan, tanf, tan, CUDA_VEC_MATH_FUNC)

__CL_CUDA_FP_MATH_FUNCS(DECLARE_CUDA_MATH_FUNCS)

#endif
