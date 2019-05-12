project(dashprint)
cmake_minimum_required(VERSION 3.1)

option(WITH_UDEV "Enable udev-based device detection" ON)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

if (WITH_UDEV)
    add_definitions(-DWITH_UDEV=1)

    include(cmake/Findudev.cmake)

    if (NOT UDEV_FOUND)
        message(FATAL_ERROR "libudev is required")
    endif (NOT UDEV_FOUND)

    include_directories(${UDEV_INCLUDE_DIR})
endif (WITH_UDEV)

find_package(Boost 1.69 REQUIRED COMPONENTS system regex filesystem iostreams log coroutine)
include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DBOOST_COROUTINES_NO_DEPRECATION_WARNING -DBOOST_BEAST_USE_STD_STRING_VIEW)

if (NOT Boost_USE_STATIC_LIBS)
    add_definitions(-DBOOST_ALL_DYN_LINK)
endif (NOT Boost_USE_STATIC_LIBS)

find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})

add_executable(binfile src/binfile/binfile.cpp)
target_link_libraries(binfile Boost::iostreams Boost::filesystem ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/webdata.cpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/binfile" "${CMAKE_SOURCE_DIR}/web/dist" "${CMAKE_BINARY_DIR}/webdata.cpp"
    DEPENDS binfile
    COMMENT "Compressing web resources..."
)

# Detect 'mmal_core mmal_util mmal_vc_client vcos bcm_host' for RPi hi-perf camera


include_directories(${CMAKE_SOURCE_DIR}/src)

set(dashprint_sources
	src/main.cpp
    src/PrinterDiscovery.cpp
    src/StaticPrinterDiscovery.cpp
    src/web/WebServer.cpp
    src/web/WebSession.cpp
    src/web/WebRequest.cpp
    src/web/WebResponse.cpp
    src/web/WebRouter.cpp
    src/web/MultipartFormData.cpp
    src/web/WebSocketHandler.cpp
    #src/WebsocketSession.cpp
    src/api/RestApi.cpp
    src/api/OctoprintRestApi.cpp
    src/Printer.cpp
    src/PrinterManager.cpp
    src/util.cpp
    src/PrintJob.cpp
    src/PrintJob.h
    src/FileManager.cpp
    ${CMAKE_BINARY_DIR}/webdata.cpp
)

if (WITH_UDEV)
    set(dashprint_sources ${dashprint_sources} src/UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)

set(LINK_LIBRARIES ${UDEV_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
if(UNIX AND NOT APPLE)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} rt)
endif()

configure_file("${CMAKE_SOURCE_DIR}/dashprint_config.h.in" "${CMAKE_BINARY_DIR}/dashprint_config.h")
include_directories("${CMAKE_BINARY_DIR}")

#add_library(boost OBJECT ${boost_sources})

add_executable(dashprint ${dashprint_sources}) # $<TARGET_OBJECTS:boost>)
target_link_libraries(dashprint ${LINK_LIBRARIES} Boost::iostreams Boost::filesystem Boost::log Boost::regex Boost::coroutine ${ZLIB_LIBRARIES})

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/dashprint/web")
install(TARGETS dashprint DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")


if (WITH_TESTS)
    enable_testing()

    include_directories(${CMAKE_SOURCE_DIR}/src)
    add_executable(PrinterTest test/PrinterTest.cpp src/Printer.cpp src/PrintJob.cpp $<TARGET_OBJECTS:boost>)
    target_link_libraries(PrinterTest ${LINK_LIBRARIES})

    add_test(PrinterTest PrinterTest)

    ##############

    add_executable(MultipartTest test/MultipartTest.cpp src/MultipartFormData.cpp $<TARGET_OBJECTS:boost>)
    target_link_libraries(MultipartTest ${LINK_LIBRARIES})

    add_test(MultipartTest MultipartTest)
endif(WITH_TESTS)
