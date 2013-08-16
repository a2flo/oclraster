#!/bin/sh

OCLR_PLATFORM="x86"
OCLR_PLATFORM_TEST_STRING=""
if [[ $( uname -o ) != "Msys" ]]; then
	OCLR_PLATFORM_TEST_STRING=$( uname -m )
else
	OCLR_PLATFORM_TEST_STRING=$( gcc -dumpmachine | sed "s/-.*//" )
fi

case $OCLR_PLATFORM_TEST_STRING in
	"i386"|"i486"|"i586"|"i686")
		OCLR_PLATFORM="x86"
		;;
	"x86_64"|"amd64")
		OCLR_PLATFORM="x64"
		;;
	*)
		echo "unknown architecture - using "${OCLR_PLATFORM}
		exit;;
esac

declare -a paths=( cl core oclraster hash pipeline program threading )
case $( uname | tr [:upper:] [:lower:] ) in
	"linux"|[a-z0-9]*"BSD")
		OCLR_INCLUDE_PATH="/usr/local/include"
		OCLR_LIB_PATH="/usr/local/lib"
	
		# remove old files and folders
		rm -Rf ${OCLR_INCLUDE_PATH}/oclraster
		rm -f ${OCLR_LIB_PATH}/liboclraster.so
		rm -f ${OCLR_LIB_PATH}/liboclrasterd.so
		rm -f ${OCLR_LIB_PATH}/liboclraster.a
		rm -f ${OCLR_LIB_PATH}/liboclrasterd.a
		
		# create/copy new files and folders
		mkdir ${OCLR_INCLUDE_PATH}/oclraster
		for val in ${paths[@]}; do
			mkdir -p ${OCLR_INCLUDE_PATH}/oclraster/${val}
		done
		cp *.h ${OCLR_INCLUDE_PATH}/oclraster/ 2>/dev/null
		cp *.hpp ${OCLR_INCLUDE_PATH}/oclraster/ 2>/dev/null
		for val in ${paths[@]}; do
			cp ${val}/*.h ${OCLR_INCLUDE_PATH}/oclraster/${val}/ 2>/dev/null
			cp ${val}/*.hpp ${OCLR_INCLUDE_PATH}/oclraster/${val}/ 2>/dev/null
		done
		
		cp ./../bin/liboclraster.so ${OCLR_LIB_PATH}/ 2>/dev/null
		cp ./../bin/liboclrasterd.so ${OCLR_LIB_PATH}/ 2>/dev/null
		cp ./../bin/liboclraster.a ${OCLR_LIB_PATH}/ 2>/dev/null
		cp ./../bin/liboclrasterd.a ${OCLR_LIB_PATH}/ 2>/dev/null
		;;
	"mingw"*)
		OCLR_MINGW_ROOT="/c/mingw"
		if [[ $OCLR_PLATFORM == "x64" ]]; then
			OCLR_MINGW_ROOT="/c/mingw64"
			if [[ $MINGW_ROOT ]]; then
				OCLR_MINGW_ROOT=$MINGW_ROOT
			fi
			if [[ $MINGW64_ROOT ]]; then
				OCLR_MINGW_ROOT=$MINGW64_ROOT
			fi
		else
			if [[ $MINGW_ROOT ]]; then
				OCLR_MINGW_ROOT=$MINGW_ROOT
			fi
			if [[ $MINGW32_ROOT ]]; then
				OCLR_MINGW_ROOT=$MINGW32_ROOT
			fi
		fi
		echo "# installing to "$OCLR_MINGW_ROOT" ..."
		OCLR_INCLUDE_PATH=$OCLR_MINGW_ROOT"/msys/include"
		OCLR_BIN_PATH=$OCLR_MINGW_ROOT"/bin"
		OCLR_LIB_PATH=$OCLR_MINGW_ROOT"/msys/lib"

		# create bin/export folder if it doesn't exist
		if [[ ! -d "${OCLR_BIN_PATH}" ]]; then
			mkdir -p ${OCLR_BIN_PATH}
		fi
	
		# remove old files and folders
		rm -Rf ${OCLR_INCLUDE_PATH}/oclraster
		rm -f ${OCLR_BIN_PATH}/oclraster.dll
		rm -f ${OCLR_BIN_PATH}/oclrasterd.dll
		rm -f ${OCLR_LIB_PATH}/liboclraster.a
		rm -f ${OCLR_LIB_PATH}/liboclrasterd.a
		
		# create/copy new files and folders
		mkdir ${OCLR_INCLUDE_PATH}/oclraster
		for val in ${paths[@]}; do
			mkdir -p ${OCLR_INCLUDE_PATH}/oclraster/${val}
		done
		cp *.h ${OCLR_INCLUDE_PATH}/oclraster/ 2>/dev/null
		cp *.hpp ${OCLR_INCLUDE_PATH}/oclraster/ 2>/dev/null
		for val in ${paths[@]}; do
			cp ${val}/*.h ${OCLR_INCLUDE_PATH}/oclraster/${val}/ 2>/dev/null
			cp ${val}/*.hpp ${OCLR_INCLUDE_PATH}/oclraster/${val}/ 2>/dev/null
		done
		
		cp ./../bin/oclraster.dll ${OCLR_BIN_PATH}/ 2>/dev/null
		cp ./../bin/oclrasterd.dll ${OCLR_BIN_PATH}/ 2>/dev/null
		cp ./../bin/liboclraster.a ${OCLR_LIB_PATH}/ 2>/dev/null
		cp ./../bin/liboclrasterd.a ${OCLR_LIB_PATH}/ 2>/dev/null
		;;
	*)
		echo "unknown operating system - exiting"
		exit
		;;
esac

echo ""
echo "#########################################################"
echo "# oclraster has been installed!"
echo "#########################################################"
echo ""
