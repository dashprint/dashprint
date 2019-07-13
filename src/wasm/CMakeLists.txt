project(wasm)

set(CMAKE_C_FLAGS "--target=wasm32-wasi -Os -flto -fvisibility=hidden -ffunction-sections -fdata-sections --sysroot=/opt/wasi-sdk/share/sysroot")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_LINKER ${WASM_LD_LINKER})
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-entry -Wl,--strip-all -Wl,-export-dynamic -Wl,--initial-memory=131072 -Wl,--lto-O3 -Wl,--gc-sections")

function(add_wasm_module name)
	foreach(f IN LISTS ARGN)
		set(files ${files} ${f})
	endforeach(f)
	add_library(${name} SHARED ${files})
	set_target_properties(${name} PROPERTIES SUFFIX ".wasm" NO_SONAME 1)
endfunction(add_wasm_module)

#add_subdirectory(nanolibc)
add_subdirectory(gcode-analyzer)

