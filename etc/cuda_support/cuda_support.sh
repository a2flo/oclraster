#!/bin/bash

declare -a dims=("" "2" "3" "4")
declare -a vec_access=("x" "y" "z" "w")
declare -a type_defs=(
	uchar:"uint":"uint":"i":"0u":"0xFFu":"8"
	ushort:"uint":"uint":"i":"0u":"0xFFFFu":"16"
	uint:"uint":"":"i":"0u":"0xFFFFFFFFu":"32"
	ulong:"ull":"":"i":"0":"0":"64"
	
	char:"int":"int":"i":"0xFF":"0x7F":"8"
	short:"int":"int":"i":"0xFFFF":"0x7FFF":"16"
	# note: neither 0xFFFFFFFF nor -2147483648 is interpreted as an int, only ~0 is!
	int:"int":"":"i":"~0":"0x7FFFFFFF":"32"
	long:"ll":"":"i":"0":"0":"64"
	
	float:"float":"":"f":"0":"0":"32"
	double:"double":"":"f":"0":"0":"64"
)
regex_matcher="(.*):(.*):(.*):(.*):(.*):(.*):(.*)"

VEC_HELPER_CODE=""
CONVERSION_CODE=""
for type in "${type_defs[@]}"; do
	single_type=${type%%:*}
	conversion_src_type_name=$(sed -E "s/"${regex_matcher}"/\2/" <<< "${type}")
	src_input_convert=$(sed -E "s/"${regex_matcher}"/\3/" <<< "${type}")
	conversion_src_kind=$(sed -E "s/"${regex_matcher}"/\4/" <<< "${type}")
	src_size=$(sed -E "s/"${regex_matcher}"/\7/" <<< "${type}")
	echo "type:$single_type"
	
	if [[ $src_input_convert != "" ]]; then
		src_input_convert="("${src_input_convert}")"
	fi
	
	IFS=""
	for (( i=0; i < ${#dims[@]}; i++ )); do
		component_count=$(expr $i + 1)
		vec_type=${single_type}${dims[$i]}
		
		echo ${single_type}" -> "${vec_type}
		
		VEC_HELPER_CODE+=$(clang -E -DSINGLE_TYPE=${single_type} -DVEC_TYPE=${vec_type} -DCOMPONENT_COUNT=${component_count} \
						   cuda_vector_helper_template.h | grep -v "#")
	done

	for dst_type in "${type_defs[@]}"; do
		single_dst_type=${dst_type%%:*}
		conversion_dst_type_name=$(sed -E "s/"${regex_matcher}"/\2/" <<< "${dst_type}")
		conversion_dst_kind=$(sed -E "s/"${regex_matcher}"/\4/" <<< "${dst_type}")
		clamp_low=$(sed -E "s/"${regex_matcher}"/\5/" <<< "${dst_type}")
		clamp_high=$(sed -E "s/"${regex_matcher}"/\6/" <<< "${dst_type}")
		dst_size=$(sed -E "s/"${regex_matcher}"/\7/" <<< "${dst_type}")
		
		ident_convert=0
		if [[ $single_type == $single_dst_type ]]; then
			ident_convert=1
		fi
		
		ident_kind_convert=0
		if [[ $conversion_src_kind == $conversion_dst_kind && $ident_convert == 0 ]]; then
			ident_kind_convert=1
		fi
		
		round_mode="rz"
		if [[ $conversion_dst_kind == "f" ]]; then
			round_mode="rn"
		fi
		
		CONVERSION_CODE+=$(clang -E -DSRC_TYPE=${single_type} -DDST_TYPE=${single_dst_type} -DROUND_MODE=${round_mode} -DCOMPONENT_COUNT=${component_count} \
						   -DINPUT_CONVERT=${src_input_convert} -DIDENT_CONVERT=${ident_convert} -DIDENT_KIND_CONVERT=${ident_kind_convert} \
						   -DCONV_SRC_TYPE=${conversion_src_type_name} -DCONV_DST_TYPE=${conversion_dst_type_name} \
						   -DCLAMP_LOW=${clamp_low} -DCLAMP_HIGH=${clamp_high} -DSRC_SIZE=${src_size} -DDST_SIZE=${dst_size} \
						   cuda_conversion_template.h | grep -v "#")
		CONVERSION_CODE+="\n"
	done
	CONVERSION_CODE+="\n"
	
	IFS=" "
done

# second pass - necessary for additional conversion functions
CONVERSION_CODE+="\n"
declare -a conv_suffix=("" "_sat")
for type in "${type_defs[@]}"; do
	single_type=${type%%:*}
	conversion_src_type_name=$(sed -E "s/"${regex_matcher}"/\2/" <<< "${type}")
	src_input_convert=$(sed -E "s/"${regex_matcher}"/\3/" <<< "${type}")
	conversion_src_kind=$(sed -E "s/"${regex_matcher}"/\4/" <<< "${type}")
	echo "conversion:$single_type"

	# convert_*
	for (( suf=0; suf < ${#conv_suffix[@]}; suf++ )); do
		suffix=${conv_suffix[$suf]}
		CONVERSION_CODE+="#define convert_${single_type}${suffix}(vec) convert_cuda_type<${single_type}, ${suf}>(vec)\n"
		CONVERSION_CODE+="#define convert_${single_type}2${suffix}(vec) convert_cuda_type2<${single_type}2, ${single_type}, ${suf}>(vec)\n"
		CONVERSION_CODE+="#define convert_${single_type}3${suffix}(vec) convert_cuda_type3<${single_type}3, ${single_type}, ${suf}>(vec)\n"
		CONVERSION_CODE+="#define convert_${single_type}4${suffix}(vec) convert_cuda_type4<${single_type}4, ${single_type}, ${suf}>(vec)\n"
		#for (( i=2; i <= 4; i++ )); do
		#	CONVERSION_CODE+="#define convert_"${single_type}${i}${suffix}"(vec) make_"${single_type}${i}"("
		#	for (( j=0; j < $i; j++ )); do
		#		CONVERSION_CODE+="convert_cuda_type<"${single_type}", "${suf}">(vec."${vec_access[$j]}")"
		#		if [[ $j < $(expr $i - 1) ]]; then
		#			CONVERSION_CODE+=", "
		#		fi
		#	done
		#	CONVERSION_CODE+=")\n"
		#done
	done
	
	# as_typen
	for (( i=0; i < ${#dims[@]}; i++ )); do
		vec_type=${single_type}${dims[$i]}
		# note: this is not standard conformant (as OpenCL is more restrictive than this, requiring the src and dst type to have the same amounts of bits)
		# however, it is expected that the incoming OpenCL source code is standard conformant, thus not requiring this check
		CONVERSION_CODE+="#define as_"${vec_type}"(vec) as_typen<"${vec_type}">(vec)\n"
	done
done


# remove empty lines
VEC_HELPER_CODE=$(echo -e ${VEC_HELPER_CODE} | grep -v "^$")
CONVERSION_CODE=$(echo -e ${CONVERSION_CODE} | grep -v "^$")

# create and fill final output files
IFS=""

# oclr_cuda_vector_helper
echo -e "// NOTE: this is an automatically generated file!\n// If you need to change anything in here, please have a look at etc/cuda_support/cuda_support.sh" > oclr_cuda_vector_helper.h
echo -e "// don't include this header on it's own, but rather include oclr_cuda_base.h\n" >> oclr_cuda_vector_helper.h
echo -e "#ifndef __OCLRASTER_CUDA_VECTOR_HELPER_H__\n#define __OCLRASTER_CUDA_VECTOR_HELPER_H__\n" >> oclr_cuda_vector_helper.h
echo -e "template<class vec_type, size_t target_size> struct vector_mapping {\n\ttypedef void type;\n\ttypedef void src_type;\n\tstatic const size_t vec_size = 1;\n\tstatic const size_t src_vec_size = 1;\n};\n" >> oclr_cuda_vector_helper.h
echo -e $VEC_HELPER_CODE >> oclr_cuda_vector_helper.h
echo -e "\n#endif" >> oclr_cuda_vector_helper.h

# oclr_cuda_conversion
echo -e "// NOTE: this is an automatically generated file!\n// If you need to change anything in here, please have a look at etc/cuda_support/cuda_support.sh" > oclr_cuda_conversion.h
echo -e "// don't include this header on it's own, but rather include oclr_cuda_base.h\n" >> oclr_cuda_conversion.h
echo -e "#ifndef __OCLRASTER_CUDA_CONVERSION_H__\n#define __OCLRASTER_CUDA_CONVERSION_H__\n" >> oclr_cuda_conversion.h
echo -e "template<class dst_type, size_t saturated_convert, class src_type> OCLRASTER_FUNC dst_type convert_cuda_type(const src_type val) { /* fail here if not specialized */ }\n" >> oclr_cuda_conversion.h
echo -e "template<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type2(const src_typen val) {\n\tdst_typen dst;\n\tdst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);\n\tdst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);\n\treturn dst;\n}\ntemplate<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type3(const src_typen val) {\n\tdst_typen dst;\n\tdst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);\n\tdst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);\n\tdst.z = convert_cuda_type<dst_type, saturated_convert, src_type>(val.z);\n\treturn dst;\n}\ntemplate<class dst_typen, class dst_type, size_t saturated_convert, class src_typen, class src_type = typename vector_mapping<src_typen, 1>::type> OCLRASTER_FUNC dst_typen convert_cuda_type4(const src_typen val) {\n\tdst_typen dst;\n\tdst.x = convert_cuda_type<dst_type, saturated_convert, src_type>(val.x);\n\tdst.y = convert_cuda_type<dst_type, saturated_convert, src_type>(val.y);\n\tdst.z = convert_cuda_type<dst_type, saturated_convert, src_type>(val.z);\n\tdst.w = convert_cuda_type<dst_type, saturated_convert, src_type>(val.w);\n\treturn dst;\n}\n" >> oclr_cuda_conversion.h
echo -e "template<class dst_type, class src_type, typename enable_if<sizeof(dst_type) == sizeof(src_type), int>::type = 0>\ndst_type as_typen(const src_type src) {\n\treturn *(dst_type*)&src;\n};\n" >> oclr_cuda_conversion.h
echo -e $CONVERSION_CODE >> oclr_cuda_conversion.h
echo -e "\n#endif" >> oclr_cuda_conversion.h
