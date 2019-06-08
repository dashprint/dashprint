project(binfile)
cmake_minimum_required(VERSION 3.8)

if (CMAKE_CROSSCOMPILING)
	set(NATIVE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/native")
	set(BINFILE_EXE "${NATIVE_BUILD_DIR}/binfile")
	set(BINFILE_EXE "${BINFILE_EXE}" PARENT_SCOPE)

	add_custom_command(OUTPUT "${NATIVE_BUILD_DIR}"
		COMMAND ${CMAKE_COMMAND} -E make_directory ${NATIVE_BUILD_DIR})

	add_custom_command(OUTPUT "${NATIVE_BUILD_DIR}/CMakeCache.txt"
		COMMAND ${CMAKE_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR} -DLOOKUP_DEPENDENCIES=On
		DEPENDS "${NATIVE_BUILD_DIR}"
		WORKING_DIRECTORY ${NATIVE_BUILD_DIR}
		COMMENT "Configuring native build...")

	add_custom_command(OUTPUT ${BINFILE_EXE}
		COMMAND ${CMAKE_BUILD_TOOL}
		WORKING_DIRECTORY ${NATIVE_BUILD_DIR}
		DEPENDS "${NATIVE_BUILD_DIR}/CMakeCache.txt")

	add_custom_target(binfileexe DEPENDS ${BINFILE_EXE})
else (CMAKE_CROSSCOMPILING)

	if (LOOKUP_DEPENDENCIES)
		find_package(Boost 1.62 REQUIRED COMPONENTS filesystem iostreams)
		include_directories(${Boost_INCLUDE_DIRS})

		find_package(ZLIB REQUIRED)
		include_directories(${ZLIB_INCLUDE_DIRS})
	endif (LOOKUP_DEPENDENCIES)

	add_executable(binfile binfile.cpp)
	add_custom_target(binfileexe DEPENDS binfile)

	target_link_libraries(binfile Boost::iostreams Boost::filesystem ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
	set(BINFILE_EXE "${CMAKE_CURRENT_BINARY_DIR}/binfile" PARENT_SCOPE)
endif (CMAKE_CROSSCOMPILING)

