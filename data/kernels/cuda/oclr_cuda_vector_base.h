
#ifndef __OCLRASTER_CUDA_VECTOR_BASE_H__
#define __OCLRASTER_CUDA_VECTOR_BASE_H__

// opencl type aliases
typedef unsigned char uchar;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long int ulong;
typedef ushort half;

// necessary cuda defines
#if defined(__SIZE_TYPE__)
typedef __SIZE_TYPE__ size_t;
#else
typedef unsigned long int size_t;
#endif
#if defined(__cplusplus)
namespace std {
	using ::size_t;
};
#endif

#define cuda_device __attribute__((always_inline, __device__))
//#define cuda_device_builtin __attribute__((__device_builtin__))
#define cuda_device_builtin
#define cuda_builtin_typedef __attribute__((__device_builtin__))

#if defined(__cplusplus)
// c++ type support (copies the c++11 stl implementation, as it's not available here)
template<class T> struct remove_const { typedef T type; };
template<class T> struct remove_const<const T> { typedef T type; };
template<class T> struct remove_volatile { typedef T type; };
template<class T> struct remove_volatile<volatile T> { typedef T type; };
template<class T> struct remove_cv {
	typedef typename remove_volatile<typename remove_const<T>::type>::type type;
};

template<class T, T v>
struct integral_constant {
	static const T value = v;
	typedef T value_type;
	typedef integral_constant type;
};
typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template<class T, class U> struct is_same : false_type {};
template<class T> struct is_same<T, T> : true_type {};

template<bool B, class T = void> struct enable_if {};
template<class T> struct enable_if<true, T> { typedef T type; };

template<class T>
struct is_floating_point : integral_constant<bool,
											 is_same<float, typename remove_cv<T>::type>::value ||
											 is_same<double, typename remove_cv<T>::type>::value> {};

template<class T>
struct is_integer : integral_constant<bool,
									  is_same<bool, typename remove_cv<T>::type>::value ||
									  is_same<char, typename remove_cv<T>::type>::value ||
									  is_same<short, typename remove_cv<T>::type>::value ||
									  is_same<int, typename remove_cv<T>::type>::value ||
									  is_same<long, typename remove_cv<T>::type>::value ||
									  is_same<long long, typename remove_cv<T>::type>::value ||
									  is_same<unsigned char, typename remove_cv<T>::type>::value ||
									  is_same<unsigned short, typename remove_cv<T>::type>::value ||
									  is_same<unsigned int, typename remove_cv<T>::type>::value ||
									  is_same<unsigned long, typename remove_cv<T>::type>::value ||
									  is_same<unsigned long long, typename remove_cv<T>::type>::value> {};

//
#define TEMPLATE_ENABLE_IF_FLOAT class T, typename enable_if<is_same<float, T>::value == 1, int>::type = 0
#define TEMPLATE_ENABLE_IF_DOUBLE class T, typename enable_if<is_same<double, T>::value == 1, int>::type = 0

template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T sqrt_T(const T& val) {
	return sqrtf(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T sqrt_T(const T& val) {
	return sqrt(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T rsqrt_T(const T& val) {
	return rsqrtf(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T rsqrt_T(const T& val) {
	return rsqrt(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T fast_sqrt_T(const T& val) {
	return __fsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T fast_sqrt_T(const T& val) {
	return __dsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_FLOAT> cuda_device T fast_rsqrt_T(const T& val) {
	return __frsqrt_rn(val);
}
template<TEMPLATE_ENABLE_IF_DOUBLE> cuda_device T fast_rsqrt_T(const T& val) {
	return __drsqrt_rn(val);
}

#endif

#endif
