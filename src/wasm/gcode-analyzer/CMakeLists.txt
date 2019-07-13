project(gcode-analyzer)

set(sources
	analyzer.cpp)

add_wasm_module(gcode-analyzer ${sources})
#target_link_libraries(gcode-analyzer nanolibc)

