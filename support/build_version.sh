#!/bin/sh

# determine the number of source files that have a timestamp newer than build_version
NEW_SOURCE_COUNT=$(find . -type f \( -name '*.h' -o -name '*.cpp' \) -newer build_version | grep -v "build_version.h" | wc -l | tr -cd [:digit:])

BUILD_VERSION=$(cat build_version)
if [ ${NEW_SOURCE_COUNT} -gt 0 ]; then
        # if this is > 0, increase build version
        BUILD_VERSION=$(expr ${BUILD_VERSION} + 1)
        echo "${BUILD_VERSION}" > build_version
        echo "# increased build version to ${BUILD_VERSION}"
fi;

# generate build_version.h if it's nonexistent or build version was increased
if [ ! -f build_version.h -o ${NEW_SOURCE_COUNT} -gt 0 ]; then
        echo "#define OCLRASTER_SUPPORT_BUILD_VERSION ${BUILD_VERSION}" > build_version.h
fi;
