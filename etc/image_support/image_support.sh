#!/bin/bash

# since OpenCL has no templates and there are _a lot_ of different image functions with different types,
# we'll have to create the image support functions beforehand (-> generate a header that contains everything)

declare -a channels=("" "2" "3" "4")
declare -a defs=(
	uchar:"float uint":") / 255.0f)"
	ushort:"float uint":") / 65535.0f)"
	uint:"uint":"))"
	ulong:"ulong":"))"
	
	char:"float int":" + 128.0f) / 255.0f) * 2.0f - 1.0f"
	short:"float int":" + 32768.0f) / 65535.0f) * 2.0f - 1.0f"
	int:"int":"))"
	long:"long":"))"
	
	oclr_half:"float":"))"
	float:"float":"))"
	double:"double":"))"
)

CODE=""
CLEAR_CODE=""
for type in "${defs[@]}"; do
	img_type=${type%%:*}
	img_norm=${type#*:*:}
	declare -a return_types=($(sed -E "s/(.*):(.*):(.*)/\2/" <<< ${type}))
	
	# read functions
	for return_type in "${return_types[@]}"; do
		IFS=""
		for (( i=0; i < ${#channels[@]}; i++ )); do
			return_type_vec=${return_type}${channels[$i]}
			return_type_vec4=${return_type}"4"
			img_type_vec=${img_type}${channels[$i]}
			echo ${img_type_vec}" -> "${return_type_vec}
			
			img_normalization=${img_norm}
			if [[ ${return_type} != "float" ]]; then
				img_normalization="))"
			fi
			
			img_zero="0"
			img_one="1"
			if [[ ${return_type} == "float" ]]; then
				img_zero="0.0f"
				img_one="1.0f"
			fi
			if [[ ${return_type} == "double" ]]; then
				img_zero="0.0"
				img_one="1.0"
			fi
			
			vec4_fill=""
			case "$i" in
				"0") vec4_fill=", "${img_zero}", "${img_zero}", "${img_one} ;;
				"1") vec4_fill=", "${img_zero}", "${img_one} ;;
				"2") vec4_fill=", "${img_one} ;;
			esac
			
			func_return_name="_"${return_type}
			if [[ ${return_type} == "float" ]]; then
				# functions without a type name are implicitly float functions
				func_return_name=""
			fi
			
			template_file="image_support_template.h"
			if [[ ${img_type} == "oclr_half" ]]; then
				template_file="image_support_template_fp16.h"
			fi
			
			img_type_vec_upper=$(echo "${img_type_vec}" | tr "[:lower:]" "[:upper:]")
			CODE+="#if defined(OCLRASTER_IMAGE_${img_type_vec_upper})\n"
			CODE+=$(clang -E -DRETURN_TYPE=${return_type} -DRETURN_TYPE_VEC=${return_type_vec} -DRETURN_TYPE_VEC4=${return_type_vec4} -DIMG_TYPE=${img_type_vec} \
					-DIMG_CONVERT_FUNC=convert_${return_type_vec} -DIMG_NORMALIZATION="${img_normalization}" -DIMG_ZERO=${img_zero} -DIMG_ONE=${img_one} \
					-DFUNC_RETURN_NAME=${func_return_name} -DVEC4_FILL=${vec4_fill} ${template_file} -DVECN=${channels[$i]} | grep -v "#")
			CODE+="\n#endif\n\n"
		done
		IFS=" "
	done
	
	# clear functions
	for (( i=1; i <= ${#channels[@]}; i++ )); do
		img_type_vec=${img_type}${channels[$i-1]}
		echo ${img_type_vec}" clear"
		
		# fp types need special handling
		is_half_type=0
		is_float_type=0
		is_double_type=0
		if [[ ${img_type} == "oclr_half" ]]; then
			is_half_type=1
		fi
		if [[ ${img_type} == "float" ]]; then
			is_float_type=1
		fi
		if [[ ${img_type} == "double" ]]; then
			is_double_type=1
			CLEAR_CODE+="#if defined(OCLRASTER_DOUBLE_SUPPORT)\n"
		fi
		
		CLEAR_CODE+=$(clang -E -DIMG_TYPE_VEC=${img_type_vec} -DIMG_TYPE=${img_type} -DCHANNEL_COUNT=${i} \
					  -DIS_HALF_TYPE=${is_half_type} -DIS_FLOAT_TYPE=${is_float_type} -DIS_DOUBLE_TYPE=${is_double_type} \
					  framebuffer_clear_template.h | grep -v "#")
		
		if [[ ${img_type} == "double" ]]; then
			CLEAR_CODE+="\n#endif"
		fi
		CLEAR_CODE+="\n\n"
	done
done

# remove empty lines
CODE=$(sed "/^$/d" <<< ${CODE})
CLEAR_CODE=$(sed "/^$/d" <<< ${CLEAR_CODE})

# create and fill final output files (oclr_image_support.h + oclr_framebuffer_clear.h)
IFS=""

echo -e "// NOTE: this is an automatically generated file!\n// If you need to change anything in here, please have a look at etc/image_support/image_support.sh\n" > oclr_image_support.h
echo -e "#ifndef __OCLRASTER_IMAGE_SUPPORT_H__\n#define __OCLRASTER_IMAGE_SUPPORT_H__\n" >> oclr_image_support.h
echo -e $CODE >> oclr_image_support.h
echo -e "#endif" >> oclr_image_support.h

echo -e "// NOTE: this is an automatically generated file!\n// If you need to change anything in here, please have a look at etc/image_support/image_support.sh\n" > oclr_framebuffer_clear.h
echo -e "#ifndef __OCLRASTER_FRAMEBUFFER_CLEAR_H__\n#define __OCLRASTER_FRAMEBUFFER_CLEAR_H__\n" >> oclr_framebuffer_clear.h
echo -e $CLEAR_CODE >> oclr_framebuffer_clear.h
echo -e "#endif" >> oclr_framebuffer_clear.h
