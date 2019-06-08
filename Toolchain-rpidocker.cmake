SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR armv6l)
set(CMAKE_CROSSCOMPILING ON)
SET(CMAKE_C_COMPILER  clang-8)
SET(CMAKE_CXX_COMPILER clang++-8)
set(WASM_LD_LINKER wasm-ld-8)

SET(CMAKE_SYSROOT "/sysroot")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "/sysroot")
set(ENV{PKG_CONFIG_LIBDIR} "/sysroot/usr/lib/arm-linux-gnueabihf/pkgconfig/")
SET(CMAKE_FIND_ROOT_PATH "/sysroot")
SET(CMAKE_PREFIX_PATH "/sysroot")

SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_NATIVE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_NATIVE_CXX_FLAGS "${CMAKE_NATIVE_CXX_FLAGS}")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -target arm-linux-gnueabihf")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -target arm-linux-gnueabihf -march=armv6 -mfpu=vfp -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard --sysroot=/sysroot -fuse-ld=lld"  CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -target arm-linux-gnueabihf -march=armv6 -mfpu=vfp -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfloat-abi=hard --sysroot=/sysroot -fuse-ld=lld -stdlib=libc++"  CACHE STRING "" FORCE)

set(CMAKE_NATIVE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
set(CMAKE_NATIVE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -target arm-linux-gnueabihf -fuse-ld=lld --sysroot=/sysroot")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -target arm-linux-gnueabihf -fuse-ld=lld --sysroot=/sysroot")

set(CMAKE_NATIVE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -nostdlib++ /sysroot/usr/local/lib/libc++.a /sysroot/usr/local/lib/libc++abi.a -Wl,--no-whole-archive -lpthread")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_INCLUDE_DIR /sysroot/usr/local/include)


