#!/bin/sh

if [ $# != 1 ]; then
	echo "Usage example:"
	echo "mkdir rpibuild && cd rpibuild"
	echo "../rpibuild.sh .."
	exit 1
fi

set -e

SRC_ROOT=$(realpath "$1")
BUILD_DIR=${PWD}

echo "Will cdinto ${BUILD_DIR}, src root is ${SRC_ROOT}"

docker run --rm=true -i --user $UID -v ${SRC_ROOT}:${SRC_ROOT} lubosd/dashprint-rpidocker:1.0 /bin/bash -s <<END
set -e
#(cd ${SRC_ROOT}/web && ng build)
cd ${BUILD_DIR}
#rm -f webdata.cpp
if [ ! -f Makefile ]; then
	cmake ${SRC_ROOT} -DCMAKE_TOOLCHAIN_FILE=${SRC_ROOT}/Toolchain-rpidocker.cmake -DCMAKE_MAKE_PROGRAM=make
		#-DUDEV_LIBRARY="/sysroot/lib/arm-linux-gnueabihf/libudev.so.1" -DUDEV_INCLUDE_DIR="/sysroot/usr/include" \
		#-DBOOST_ROOT="/sysroot/usr/local" \
		#-DZLIB_LIBRARY="/sysroot/usr/lib/arm-linux-gnueabihf/libz.so" -DZLIB_INCLUDE_DIR="/sysroot/usr/include"
fi
make -j4
END

