project(pi3print)
cmake_minimum_required(VERSION 3.1)

option(WITH_UDEV "Enable udev-based device detection" ON)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads)

if (WITH_UDEV)
    add_definitions(-DWITH_UDEV=1)

    include(cmake/Findudev.cmake)

    if (NOT UDEV_FOUND)
        status(FATAL_ERROR "libudev is required")
    endif (NOT UDEV_FOUND)

    include_directories(${UDEV_INCLUDE_DIR})
endif (WITH_UDEV)

# Boost is typically WAY too old on Raspbian, so we carry our own version
# which is used as a header-only library.
#find_package(Boost 1.55.0 REQUIRED system)

add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_CHRONO_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED)

set(pi3print_sources
	src/main.cpp
    src/PrinterDiscovery.cpp
    src/StaticPrinterDiscovery.cpp
    src/WebServer.cpp
    src/WebSession.cpp
    src/WebRESTHandler.cpp
)

# Compile Boost.Filesystem
set(pi3print_sources ${pi3print_sources}
    src/boost/libs/filesystem/src/path.cpp
    src/boost/libs/filesystem/src/operations.cpp
    src/boost/libs/filesystem/src/codecvt_error_category.cpp
    src/boost/libs/filesystem/src/path_traits.cpp
    src/boost/libs/filesystem/src/portability.cpp
    src/boost/libs/filesystem/src/unique_path.cpp
    src/boost/libs/filesystem/src/utf8_codecvt_facet.cpp
)

FILE(GLOB children ${CMAKE_SOURCE_DIR}/src/boost/libs/*)
FOREACH(child ${children})
	IF(IS_DIRECTORY ${child})
		include_directories(${child}/include)
	ENDIF()
ENDFOREACH()

#include_directories(
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/asio/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/beast/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/property_tree/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/system/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/core/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/config/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/utility/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/type_traits/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/smart_ptr/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/endian/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/iterator/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/detail/include
#    ${CMAKE_SOURCE_DIR}/src/boost/libs/multi_index/include
#)

if (WITH_UDEV)
    set(pi3print_sources ${pi3print_sources} src/UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)

configure_file("${CMAKE_SOURCE_DIR}/pi3print_config.h.in" "${CMAKE_BINARY_DIR}/pi3print_config.h")
include_directories("${CMAKE_BINARY_DIR}")

add_executable(pi3print ${pi3print_sources})
target_link_libraries(pi3print ${Boost_LIBRARIES} ${UDEV_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pi3print/web")
install(TARGETS pi3print DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
