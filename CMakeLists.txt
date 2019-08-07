project(dashprint)
cmake_minimum_required(VERSION 3.1)

option(WITH_UDEV "Enable udev-based device detection" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

find_package(Threads)

if (WITH_UDEV)
    add_definitions(-DWITH_UDEV=1)

    include(cmake/Findudev.cmake)

    if (NOT UDEV_FOUND)
        message(FATAL_ERROR "libudev is required")
    endif (NOT UDEV_FOUND)

    include_directories(${UDEV_INCLUDE_DIR})
endif (WITH_UDEV)

if (NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB)
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_MULTITHREADED ON)
    set(Boost_USE_STATIC_RUNTIME OFF)
    set(BUILD_SHARED_LIBS TRUE)
endif (NOT Boost_USE_STATIC_LIBS)

find_package(Boost 1.69 REQUIRED COMPONENTS system regex filesystem iostreams log coroutine)
include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_BEAST_USE_STD_STRING_VIEW)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

find_package(OpenSSL REQUIRED)

set(LINK_LIBRARIES ${UDEV_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES} Boost::iostreams Boost::filesystem Boost::log Boost::regex Boost::coroutine)
if(UNIX AND NOT APPLE)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} rt)
endif()

find_package(bcmhost)
if (BCMHOST_FOUND)
    include_directories(${BCMHOST_INCLUDE_DIR})
    set(LINK_LIBRARIES ${LINK_LIBRARIES} ${BCMHOST_LIBRARY} ${MMAL_LIBRARY}
        ${MMAL_COMPONENTS_LIBRARY} ${MMAL_CORE_LIBRARY} ${MMAL_UTIL_LIBRARY} ${VCOS_LIBRARY} ${VCHIQ_LIBRARY})

    add_definitions(-DWITH_MMAL_CAMERA)
endif (BCMHOST_FOUND)

find_package(avformat REQUIRED)
set(LINK_LIBRARIES ${LINK_LIBRARIES} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY})

configure_file("${CMAKE_SOURCE_DIR}/dashprint_config.h.in" "${CMAKE_BINARY_DIR}/dashprint_config.h")
include_directories("${CMAKE_BINARY_DIR}")

if (WITH_TESTS)
    enable_testing()

    include_directories(${CMAKE_SOURCE_DIR}/src)
    add_executable(PrinterTest test/PrinterTest.cpp src/Printer.cpp src/PrintJob.cpp)
    target_link_libraries(PrinterTest ${LINK_LIBRARIES})

    add_test(PrinterTest PrinterTest)

    ##############

    add_executable(MultipartTest test/MultipartTest.cpp src/web/MultipartFormData.cpp)
    target_link_libraries(MultipartTest ${LINK_LIBRARIES})

    add_test(MultipartTest MultipartTest)
endif(WITH_TESTS)

add_subdirectory(src)

