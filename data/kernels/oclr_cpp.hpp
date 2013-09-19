
#ifndef __OCLRASTER_CPP_HPP__
#define __OCLRASTER_CPP_HPP__

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
#if !defined(PLATFORM_AMD)
	static const T value = v;
#else
	enum { value = v };
#endif
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
#if !defined(PLATFORM_AMD)
									  is_same<long long, typename remove_cv<T>::type>::value ||
#endif
									  is_same<unsigned char, typename remove_cv<T>::type>::value ||
									  is_same<unsigned short, typename remove_cv<T>::type>::value ||
									  is_same<unsigned int, typename remove_cv<T>::type>::value ||
									  is_same<unsigned long, typename remove_cv<T>::type>::value
#if !defined(PLATFORM_AMD)
									  || is_same<unsigned long long, typename remove_cv<T>::type>::value
#endif
									  > {};

#endif
