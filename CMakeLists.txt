project(pi3print)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)

include(cmake/Findudev.cmake)

if (NOT UDEV_FOUND)
    status(FATAL_ERROR "libudev is required")
endif (NOT UDEV_FOUND)

include_directories(${UDEV_INCLUDE_DIR})

find_package(Boost 1.55.0 REQUIRED system)

set(pi3print_sources
	src/main.cpp
    src/PrinterDiscovery.cpp
)

add_executable(pi3print ${pi3print_sources})
target_link_libraries(pi3print ${Boost_LIBRARIES} ${UDEV_LIBRARY}) 

