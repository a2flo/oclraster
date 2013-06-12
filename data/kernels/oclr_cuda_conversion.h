// NOTE: this is an automatically generated file!
// If you need to change anything in here, please have a look at etc/cuda_support/cuda_support.sh
// don't include this header on it's own, but rather include oclr_cuda_base.h

#ifndef __OCLRASTER_CUDA_CONVERSION_H__
#define __OCLRASTER_CUDA_CONVERSION_H__

template<class dst_type, size_t saturated_convert, class src_type> OCLRASTER_FUNC dst_type convert_cuda_type(const src_type val) { /* fail here if not specialized */ }

template<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type2(const src_typen val) {
	dst_typen dst;
	dst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);
	dst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);
	return dst;
}
template<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type3(const src_typen val) {
	dst_typen dst;
	dst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);
	dst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);
	dst.z = convert_cuda_type<dst_type, saturated_convert, src_type>(val.z);
	return dst;
}
template<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type4(const src_typen val) {
	dst_typen dst;
	dst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);
	dst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);
	dst.z = convert_cuda_type<dst_type, saturated_convert, src_type>(val.z);
	dst.w = convert_cuda_type<dst_type, saturated_convert, src_type>(val.w);
	return dst;
}

template<class dst_type, class src_type, typename enable_if<sizeof(dst_type) == sizeof(src_type), int>::type = 0>
dst_type as_typen(const src_type src) {
	return *(dst_type*)&src;
};

template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, uchar>(const uchar val) { return val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, uchar>(const uchar val) { return val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, uchar>(const uchar val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, uchar>(const uchar val) { return (ushort)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, uchar>(const uchar val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, uchar>(const uchar val) { return (uint)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, uchar>(const uchar val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, uchar>(const uchar val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, uchar>(const uchar val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, uchar>(const uchar val) { return (char)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, uchar>(const uchar val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, uchar>(const uchar val) { return (short)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, uchar>(const uchar val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, uchar>(const uchar val) { return (int)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, uchar>(const uchar val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, uchar>(const uchar val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, uchar>(const uchar val) { return (float)__uint2float_rn((uint)val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, uchar>(const uchar val) { return (float)__uint2float_rn((uint)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, uchar>(const uchar val) { return (double)__uint2double_rn((uint)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, uchar>(const uchar val) { return (double)__uint2double_rn((uint)val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, ushort>(const ushort val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, ushort>(const ushort val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, ushort>(const ushort val) { return val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, ushort>(const ushort val) { return val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, ushort>(const ushort val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, ushort>(const ushort val) { return (uint)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, ushort>(const ushort val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, ushort>(const ushort val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, ushort>(const ushort val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, ushort>(const ushort val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, ushort>(const ushort val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, ushort>(const ushort val) { return (short)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, ushort>(const ushort val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, ushort>(const ushort val) { return (int)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, ushort>(const ushort val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, ushort>(const ushort val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, ushort>(const ushort val) { return (float)__uint2float_rn((uint)val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, ushort>(const ushort val) { return (float)__uint2float_rn((uint)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, ushort>(const ushort val) { return (double)__uint2double_rn((uint)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, ushort>(const ushort val) { return (double)__uint2double_rn((uint)val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, uint>(const uint val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, uint>(const uint val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, uint>(const uint val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, uint>(const uint val) { return (ushort)clamp(val, 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, uint>(const uint val) { return val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, uint>(const uint val) { return val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, uint>(const uint val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, uint>(const uint val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, uint>(const uint val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, uint>(const uint val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, uint>(const uint val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, uint>(const uint val) { return (short)clamp(val, 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, uint>(const uint val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, uint>(const uint val) { return (int)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, uint>(const uint val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, uint>(const uint val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, uint>(const uint val) { return (float)__uint2float_rn(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, uint>(const uint val) { return (float)__uint2float_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, uint>(const uint val) { return (double)__uint2double_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, uint>(const uint val) { return (double)__uint2double_rn(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, ulong>(const ulong val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, ulong>(const ulong val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, ulong>(const ulong val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, ulong>(const ulong val) { return (ushort)clamp(val, 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, ulong>(const ulong val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, ulong>(const ulong val) { return (uint)clamp(val, 0u, 0xFFFFFFFFu); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, ulong>(const ulong val) { return val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, ulong>(const ulong val) { return val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, ulong>(const ulong val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, ulong>(const ulong val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, ulong>(const ulong val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, ulong>(const ulong val) { return (short)clamp(val, 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, ulong>(const ulong val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, ulong>(const ulong val) { return (int)clamp(val, ~0, 0x7FFFFFFF); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, ulong>(const ulong val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, ulong>(const ulong val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, ulong>(const ulong val) { return (float)__ull2float_rn(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, ulong>(const ulong val) { return (float)__ull2float_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, ulong>(const ulong val) { return (double)__ull2double_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, ulong>(const ulong val) { return (double)__ull2double_rn(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, char>(const char val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, char>(const char val) { return (uchar)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, char>(const char val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, char>(const char val) { return (ushort)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, char>(const char val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, char>(const char val) { return (uint)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, char>(const char val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, char>(const char val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, char>(const char val) { return val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, char>(const char val) { return val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, char>(const char val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, char>(const char val) { return (short)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, char>(const char val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, char>(const char val) { return (int)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, char>(const char val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, char>(const char val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, char>(const char val) { return (float)__int2float_rn((int)val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, char>(const char val) { return (float)__int2float_rn((int)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, char>(const char val) { return (double)__int2double_rn((int)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, char>(const char val) { return (double)__int2double_rn((int)val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, short>(const short val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, short>(const short val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, short>(const short val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, short>(const short val) { return (ushort)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, short>(const short val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, short>(const short val) { return (uint)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, short>(const short val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, short>(const short val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, short>(const short val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, short>(const short val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, short>(const short val) { return val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, short>(const short val) { return val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, short>(const short val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, short>(const short val) { return (int)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, short>(const short val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, short>(const short val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, short>(const short val) { return (float)__int2float_rn((int)val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, short>(const short val) { return (float)__int2float_rn((int)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, short>(const short val) { return (double)__int2double_rn((int)val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, short>(const short val) { return (double)__int2double_rn((int)val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, int>(const int val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, int>(const int val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, int>(const int val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, int>(const int val) { return (ushort)clamp(val, 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, int>(const int val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, int>(const int val) { return (uint)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, int>(const int val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, int>(const int val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, int>(const int val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, int>(const int val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, int>(const int val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, int>(const int val) { return (short)clamp(val, 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, int>(const int val) { return val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, int>(const int val) { return val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, int>(const int val) { return (long)val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, int>(const int val) { return (long)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, int>(const int val) { return (float)__int2float_rn(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, int>(const int val) { return (float)__int2float_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, int>(const int val) { return (double)__int2double_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, int>(const int val) { return (double)__int2double_rn(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, long>(const long val) { return (uchar)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, long>(const long val) { return (uchar)clamp(val, 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, long>(const long val) { return (ushort)val; }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, long>(const long val) { return (ushort)clamp(val, 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, long>(const long val) { return (uint)val; }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, long>(const long val) { return (uint)clamp(val, 0u, 0xFFFFFFFFu); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, long>(const long val) { return (ulong)val; }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, long>(const long val) { return (ulong)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, long>(const long val) { return (char)val; }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, long>(const long val) { return (char)clamp(val, 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, long>(const long val) { return (short)val; }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, long>(const long val) { return (short)clamp(val, 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, long>(const long val) { return (int)val; }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, long>(const long val) { return (int)clamp(val, ~0, 0x7FFFFFFF); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, long>(const long val) { return val; }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, long>(const long val) { return val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, long>(const long val) { return (float)__ll2float_rn(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, long>(const long val) { return (float)__ll2float_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, long>(const long val) { return (double)__ll2double_rn(val); }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, long>(const long val) { return (double)__ll2double_rn(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, float>(const float val) { return (uchar)__float2uint_rz(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, float>(const float val) { return (uchar)clamp(__float2uint_rz(val), 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, float>(const float val) { return (ushort)__float2uint_rz(val); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, float>(const float val) { return (ushort)clamp(__float2uint_rz(val), 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, float>(const float val) { return (uint)__float2uint_rz(val); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, float>(const float val) { return (uint)__float2uint_rz(val); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, float>(const float val) { return (ulong)__float2ull_rz(val); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, float>(const float val) { return (ulong)__float2ull_rz(val); }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, float>(const float val) { return (char)__float2int_rz(val); }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, float>(const float val) { return (char)clamp(__float2int_rz(val), 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, float>(const float val) { return (short)__float2int_rz(val); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, float>(const float val) { return (short)clamp(__float2int_rz(val), 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, float>(const float val) { return (int)__float2int_rz(val); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, float>(const float val) { return (int)__float2int_rz(val); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, float>(const float val) { return (long)__float2ll_rz(val); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, float>(const float val) { return (long)__float2ll_rz(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, float>(const float val) { return val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, float>(const float val) { return val; }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, float>(const float val) { return (double)val; }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, float>(const float val) { return (double)val; }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 0, double>(const double val) { return (uchar)__double2uint_rz(val); }
template<> OCLRASTER_FUNC uchar convert_cuda_type<uchar, 1, double>(const double val) { return (uchar)clamp(__double2uint_rz(val), 0u, 0xFFu); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 0, double>(const double val) { return (ushort)__double2uint_rz(val); }
template<> OCLRASTER_FUNC ushort convert_cuda_type<ushort, 1, double>(const double val) { return (ushort)clamp(__double2uint_rz(val), 0u, 0xFFFFu); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 0, double>(const double val) { return (uint)__double2uint_rz(val); }
template<> OCLRASTER_FUNC uint convert_cuda_type<uint, 1, double>(const double val) { return (uint)clamp(__double2uint_rz(val), 0u, 0xFFFFFFFFu); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 0, double>(const double val) { return (ulong)__double2ull_rz(val); }
template<> OCLRASTER_FUNC ulong convert_cuda_type<ulong, 1, double>(const double val) { return (ulong)__double2ull_rz(val); }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 0, double>(const double val) { return (char)__double2int_rz(val); }
template<> OCLRASTER_FUNC char convert_cuda_type<char, 1, double>(const double val) { return (char)clamp(__double2int_rz(val), 0xFF, 0x7F); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 0, double>(const double val) { return (short)__double2int_rz(val); }
template<> OCLRASTER_FUNC short convert_cuda_type<short, 1, double>(const double val) { return (short)clamp(__double2int_rz(val), 0xFFFF, 0x7FFF); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 0, double>(const double val) { return (int)__double2int_rz(val); }
template<> OCLRASTER_FUNC int convert_cuda_type<int, 1, double>(const double val) { return (int)clamp(__double2int_rz(val), ~0, 0x7FFFFFFF); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 0, double>(const double val) { return (long)__double2ll_rz(val); }
template<> OCLRASTER_FUNC long convert_cuda_type<long, 1, double>(const double val) { return (long)__double2ll_rz(val); }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 0, double>(const double val) { return (float)val; }
template<> OCLRASTER_FUNC float convert_cuda_type<float, 1, double>(const double val) { return (float)val; }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 0, double>(const double val) { return val; }
template<> OCLRASTER_FUNC double convert_cuda_type<double, 1, double>(const double val) { return val; }
#define convert_uchar(vec) convert_cuda_type<uchar, 0>(vec)
#define convert_uchar2(vec) convert_cuda_type2<uchar2, uchar, 0>(vec)
#define convert_uchar3(vec) convert_cuda_type3<uchar3, uchar, 0>(vec)
#define convert_uchar4(vec) convert_cuda_type4<uchar4, uchar, 0>(vec)
#define convert_uchar_sat(vec) convert_cuda_type<uchar, 1>(vec)
#define convert_uchar2_sat(vec) convert_cuda_type2<uchar2, uchar, 1>(vec)
#define convert_uchar3_sat(vec) convert_cuda_type3<uchar3, uchar, 1>(vec)
#define convert_uchar4_sat(vec) convert_cuda_type4<uchar4, uchar, 1>(vec)
#define as_uchar(vec) as_typen<uchar>(vec)
#define as_uchar2(vec) as_typen<uchar2>(vec)
#define as_uchar3(vec) as_typen<uchar3>(vec)
#define as_uchar4(vec) as_typen<uchar4>(vec)
#define convert_ushort(vec) convert_cuda_type<ushort, 0>(vec)
#define convert_ushort2(vec) convert_cuda_type2<ushort2, ushort, 0>(vec)
#define convert_ushort3(vec) convert_cuda_type3<ushort3, ushort, 0>(vec)
#define convert_ushort4(vec) convert_cuda_type4<ushort4, ushort, 0>(vec)
#define convert_ushort_sat(vec) convert_cuda_type<ushort, 1>(vec)
#define convert_ushort2_sat(vec) convert_cuda_type2<ushort2, ushort, 1>(vec)
#define convert_ushort3_sat(vec) convert_cuda_type3<ushort3, ushort, 1>(vec)
#define convert_ushort4_sat(vec) convert_cuda_type4<ushort4, ushort, 1>(vec)
#define as_ushort(vec) as_typen<ushort>(vec)
#define as_ushort2(vec) as_typen<ushort2>(vec)
#define as_ushort3(vec) as_typen<ushort3>(vec)
#define as_ushort4(vec) as_typen<ushort4>(vec)
#define convert_uint(vec) convert_cuda_type<uint, 0>(vec)
#define convert_uint2(vec) convert_cuda_type2<uint2, uint, 0>(vec)
#define convert_uint3(vec) convert_cuda_type3<uint3, uint, 0>(vec)
#define convert_uint4(vec) convert_cuda_type4<uint4, uint, 0>(vec)
#define convert_uint_sat(vec) convert_cuda_type<uint, 1>(vec)
#define convert_uint2_sat(vec) convert_cuda_type2<uint2, uint, 1>(vec)
#define convert_uint3_sat(vec) convert_cuda_type3<uint3, uint, 1>(vec)
#define convert_uint4_sat(vec) convert_cuda_type4<uint4, uint, 1>(vec)
#define as_uint(vec) as_typen<uint>(vec)
#define as_uint2(vec) as_typen<uint2>(vec)
#define as_uint3(vec) as_typen<uint3>(vec)
#define as_uint4(vec) as_typen<uint4>(vec)
#define convert_ulong(vec) convert_cuda_type<ulong, 0>(vec)
#define convert_ulong2(vec) convert_cuda_type2<ulong2, ulong, 0>(vec)
#define convert_ulong3(vec) convert_cuda_type3<ulong3, ulong, 0>(vec)
#define convert_ulong4(vec) convert_cuda_type4<ulong4, ulong, 0>(vec)
#define convert_ulong_sat(vec) convert_cuda_type<ulong, 1>(vec)
#define convert_ulong2_sat(vec) convert_cuda_type2<ulong2, ulong, 1>(vec)
#define convert_ulong3_sat(vec) convert_cuda_type3<ulong3, ulong, 1>(vec)
#define convert_ulong4_sat(vec) convert_cuda_type4<ulong4, ulong, 1>(vec)
#define as_ulong(vec) as_typen<ulong>(vec)
#define as_ulong2(vec) as_typen<ulong2>(vec)
#define as_ulong3(vec) as_typen<ulong3>(vec)
#define as_ulong4(vec) as_typen<ulong4>(vec)
#define convert_char(vec) convert_cuda_type<char, 0>(vec)
#define convert_char2(vec) convert_cuda_type2<char2, char, 0>(vec)
#define convert_char3(vec) convert_cuda_type3<char3, char, 0>(vec)
#define convert_char4(vec) convert_cuda_type4<char4, char, 0>(vec)
#define convert_char_sat(vec) convert_cuda_type<char, 1>(vec)
#define convert_char2_sat(vec) convert_cuda_type2<char2, char, 1>(vec)
#define convert_char3_sat(vec) convert_cuda_type3<char3, char, 1>(vec)
#define convert_char4_sat(vec) convert_cuda_type4<char4, char, 1>(vec)
#define as_char(vec) as_typen<char>(vec)
#define as_char2(vec) as_typen<char2>(vec)
#define as_char3(vec) as_typen<char3>(vec)
#define as_char4(vec) as_typen<char4>(vec)
#define convert_short(vec) convert_cuda_type<short, 0>(vec)
#define convert_short2(vec) convert_cuda_type2<short2, short, 0>(vec)
#define convert_short3(vec) convert_cuda_type3<short3, short, 0>(vec)
#define convert_short4(vec) convert_cuda_type4<short4, short, 0>(vec)
#define convert_short_sat(vec) convert_cuda_type<short, 1>(vec)
#define convert_short2_sat(vec) convert_cuda_type2<short2, short, 1>(vec)
#define convert_short3_sat(vec) convert_cuda_type3<short3, short, 1>(vec)
#define convert_short4_sat(vec) convert_cuda_type4<short4, short, 1>(vec)
#define as_short(vec) as_typen<short>(vec)
#define as_short2(vec) as_typen<short2>(vec)
#define as_short3(vec) as_typen<short3>(vec)
#define as_short4(vec) as_typen<short4>(vec)
#define convert_int(vec) convert_cuda_type<int, 0>(vec)
#define convert_int2(vec) convert_cuda_type2<int2, int, 0>(vec)
#define convert_int3(vec) convert_cuda_type3<int3, int, 0>(vec)
#define convert_int4(vec) convert_cuda_type4<int4, int, 0>(vec)
#define convert_int_sat(vec) convert_cuda_type<int, 1>(vec)
#define convert_int2_sat(vec) convert_cuda_type2<int2, int, 1>(vec)
#define convert_int3_sat(vec) convert_cuda_type3<int3, int, 1>(vec)
#define convert_int4_sat(vec) convert_cuda_type4<int4, int, 1>(vec)
#define as_int(vec) as_typen<int>(vec)
#define as_int2(vec) as_typen<int2>(vec)
#define as_int3(vec) as_typen<int3>(vec)
#define as_int4(vec) as_typen<int4>(vec)
#define convert_long(vec) convert_cuda_type<long, 0>(vec)
#define convert_long2(vec) convert_cuda_type2<long2, long, 0>(vec)
#define convert_long3(vec) convert_cuda_type3<long3, long, 0>(vec)
#define convert_long4(vec) convert_cuda_type4<long4, long, 0>(vec)
#define convert_long_sat(vec) convert_cuda_type<long, 1>(vec)
#define convert_long2_sat(vec) convert_cuda_type2<long2, long, 1>(vec)
#define convert_long3_sat(vec) convert_cuda_type3<long3, long, 1>(vec)
#define convert_long4_sat(vec) convert_cuda_type4<long4, long, 1>(vec)
#define as_long(vec) as_typen<long>(vec)
#define as_long2(vec) as_typen<long2>(vec)
#define as_long3(vec) as_typen<long3>(vec)
#define as_long4(vec) as_typen<long4>(vec)
#define convert_float(vec) convert_cuda_type<float, 0>(vec)
#define convert_float2(vec) convert_cuda_type2<float2, float, 0>(vec)
#define convert_float3(vec) convert_cuda_type3<float3, float, 0>(vec)
#define convert_float4(vec) convert_cuda_type4<float4, float, 0>(vec)
#define convert_float_sat(vec) convert_cuda_type<float, 1>(vec)
#define convert_float2_sat(vec) convert_cuda_type2<float2, float, 1>(vec)
#define convert_float3_sat(vec) convert_cuda_type3<float3, float, 1>(vec)
#define convert_float4_sat(vec) convert_cuda_type4<float4, float, 1>(vec)
#define as_float(vec) as_typen<float>(vec)
#define as_float2(vec) as_typen<float2>(vec)
#define as_float3(vec) as_typen<float3>(vec)
#define as_float4(vec) as_typen<float4>(vec)
#define convert_double(vec) convert_cuda_type<double, 0>(vec)
#define convert_double2(vec) convert_cuda_type2<double2, double, 0>(vec)
#define convert_double3(vec) convert_cuda_type3<double3, double, 0>(vec)
#define convert_double4(vec) convert_cuda_type4<double4, double, 0>(vec)
#define convert_double_sat(vec) convert_cuda_type<double, 1>(vec)
#define convert_double2_sat(vec) convert_cuda_type2<double2, double, 1>(vec)
#define convert_double3_sat(vec) convert_cuda_type3<double3, double, 1>(vec)
#define convert_double4_sat(vec) convert_cuda_type4<double4, double, 1>(vec)
#define as_double(vec) as_typen<double>(vec)
#define as_double2(vec) as_typen<double2>(vec)
#define as_double3(vec) as_typen<double3>(vec)
#define as_double4(vec) as_typen<double4>(vec)

#endif
