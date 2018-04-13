SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv6l)

# Create a chroot copied from Raspbian image. User qemu-arm built as static for binfmt to install extra packages.
# RPI_SYSROOT=/home/lubos/Install/raspbian

SET(CMAKE_C_COMPILER  clang)
SET(CMAKE_CXX_COMPILER clang++)

SET(CMAKE_SYSROOT ${RPI_SYSROOT})
SET(CMAKE_FIND_ROOT_PATH ${RPI_SYSROOT})

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -target arm-linux-gnueabihf")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target arm-linux-gnueabihf -march=armv6 -mfpu=vfp -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target arm-linux-gnueabihf -march=armv6 -mfpu=vfp -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
