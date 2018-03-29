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
    src/Printer.cpp
    src/PrinterManager.cpp
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

# Compile Boost.Log
include_directories(${CMAKE_SOURCE_DIR}/src/boost/libs/log/src)
set(pi3print_sources ${pi3print_sources}
    src/boost/libs/log/src/attribute_name.cpp
    src/boost/libs/log/src/attribute_set.cpp
    src/boost/libs/log/src/attribute_value_set.cpp
    src/boost/libs/log/src/code_conversion.cpp
    src/boost/libs/log/src/core.cpp
    src/boost/libs/log/src/date_time_format_parser.cpp
    src/boost/libs/log/src/default_attribute_names.cpp
    src/boost/libs/log/src/default_sink.cpp
    src/boost/libs/log/src/dump.cpp
    src/boost/libs/log/src/event.cpp
    src/boost/libs/log/src/exceptions.cpp
    src/boost/libs/log/src/format_parser.cpp
    src/boost/libs/log/src/global_logger_storage.cpp
    src/boost/libs/log/src/named_scope.cpp
    src/boost/libs/log/src/named_scope_format_parser.cpp
    src/boost/libs/log/src/once_block.cpp
    src/boost/libs/log/src/permissions.cpp
    src/boost/libs/log/src/process_id.cpp
    src/boost/libs/log/src/process_name.cpp
    src/boost/libs/log/src/record_ostream.cpp
    src/boost/libs/log/src/severity_level.cpp
    src/boost/libs/log/src/spirit_encoding.cpp
    src/boost/libs/log/src/syslog_backend.cpp
    src/boost/libs/log/src/text_file_backend.cpp
    src/boost/libs/log/src/text_multifile_backend.cpp
    src/boost/libs/log/src/text_ostream_backend.cpp
    src/boost/libs/log/src/thread_id.cpp
    src/boost/libs/log/src/threadsafe_queue.cpp
    src/boost/libs/log/src/thread_specific.cpp
    src/boost/libs/log/src/timer.cpp
    src/boost/libs/log/src/timestamp.cpp
    src/boost/libs/log/src/trivial.cpp
    src/boost/libs/log/src/unhandled_exception_count.cpp
    src/boost/libs/log/src/posix/ipc_reliable_message_queue.cpp
    src/boost/libs/log/src/posix/object_name.cpp
)

# Compile Boost.Thread
set(pi3print_sources ${pi3print_sources}
    src/boost/libs/thread/src/future.cpp
    src/boost/libs/thread/src/pthread/thread.cpp
    src/boost/libs/thread/src/pthread/once_atomic.cpp
    src/boost/libs/thread/src/tss_null.cpp
)

FILE(GLOB children ${CMAKE_SOURCE_DIR}/src/boost/libs/*)
FOREACH(child ${children})
	IF(IS_DIRECTORY ${child})
		include_directories(${child}/include)
	ENDIF()
ENDFOREACH()

if (WITH_UDEV)
    set(pi3print_sources ${pi3print_sources} src/UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)

configure_file("${CMAKE_SOURCE_DIR}/pi3print_config.h.in" "${CMAKE_BINARY_DIR}/pi3print_config.h")
include_directories("${CMAKE_BINARY_DIR}")

add_executable(pi3print ${pi3print_sources})
target_link_libraries(pi3print ${Boost_LIBRARIES} ${UDEV_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
if(UNIX AND NOT APPLE)
    target_link_libraries(pi3print rt)
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/pi3print/web")
install(TARGETS pi3print DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
