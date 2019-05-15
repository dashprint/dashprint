set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(toolchain "/opt/clang+llvm-8.0.0-armv7a-linux-gnueabihf")
set(CMAKE_C_COMPILER ${toolchain}/bin/clang)
set(CMAKE_CXX_COMPILER ${toolchain}/bin/clang++)
set(WASM_LD_LINKER ${toolchain}/bin/wasm-ld)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib++ -Wl,--whole-archive -l:libc++.a -l:libc++abi.a -Wl,--no-whole-archive -lpthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(Boost_USE_STATIC_LIBS ON)

