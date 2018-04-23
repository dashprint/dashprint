project(dashprint)
cmake_minimum_required(VERSION 3.1)

option(WITH_UDEV "Enable udev-based device detection" ON)

set(CMAKE_CXX_STANDARD 11)
enable_language(ASM)

find_package(Threads)

if (WITH_UDEV)
    add_definitions(-DWITH_UDEV=1)

    include(cmake/Findudev.cmake)

    if (NOT UDEV_FOUND)
        message(FATAL_ERROR "libudev is required")
    endif (NOT UDEV_FOUND)

    include_directories(${UDEV_INCLUDE_DIR})
endif (WITH_UDEV)

# Boost is typically WAY too old on Raspbian, so we carry our own version
# which is used as a header-only library.
#find_package(Boost 1.55.0 REQUIRED system)

add_definitions(-DBOOST_ALL_NO_LIB -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_CHRONO_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_BEAST_USE_POSIX_FILE=1 -DBOOST_COROUTINES_NO_DEPRECATION_WARNING)

set(dashprint_sources
	src/main.cpp
    src/PrinterDiscovery.cpp
    src/StaticPrinterDiscovery.cpp
    src/WebServer.cpp
    src/WebSession.cpp
    src/WebsocketSession.cpp
    src/WebRESTHandler.cpp
    src/Printer.cpp
    src/PrinterManager.cpp
    src/util.cpp
        src/PrintJob.cpp src/PrintJob.h)

include_directories(${CMAKE_SOURCE_DIR}/src/boost/libs/numeric/conversion/include)

# Compile Boost.Filesystem
set(boost_sources
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
set(boost_sources ${boost_sources}
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
set(boost_sources ${boost_sources}
    src/boost/libs/thread/src/future.cpp
    src/boost/libs/thread/src/pthread/thread.cpp
    src/boost/libs/thread/src/pthread/once_atomic.cpp
    src/boost/libs/thread/src/tss_null.cpp
)

# Compile Boost.Coroutine
set(boost_sources ${boost_sources}
    src/boost/libs/coroutine/src/detail/coroutine_context.cpp
    src/boost/libs/coroutine/src/posix/stack_traits.cpp
    src/boost/libs/coroutine/src/exceptions.cpp
)

# Compile Context
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "(armv6l|armv7l)")
    set(boost_sources ${boost_sources}
        src/boost/libs/context/src/asm/jump_arm_aapcs_elf_gas.S
        src/boost/libs/context/src/asm/make_arm_aapcs_elf_gas.S
        src/boost/libs/context/src/asm/ontop_arm_aapcs_elf_gas.S
    )
elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "(x86|i386|i686|x86_64|amd64)")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(boost_sources ${boost_sources}
            src/boost/libs/context/src/asm/jump_x86_64_sysv_elf_gas.S
            src/boost/libs/context/src/asm/make_x86_64_sysv_elf_gas.S
            src/boost/libs/context/src/asm/ontop_x86_64_sysv_elf_gas.S
        )
    else()
        set(boost_sources ${boost_sources}
            src/boost/libs/context/src/asm/jump_i386_sysv_elf_gas.S
            src/boost/libs/context/src/asm/make_i386_sysv_elf_gas.S
            src/boost/libs/context/src/asm/ontop_i386sysv_elf_gas.S
            )
    endif()
endif()

FILE(GLOB children ${CMAKE_SOURCE_DIR}/src/boost/libs/*)
FOREACH(child ${children})
	IF(IS_DIRECTORY ${child})
		include_directories(${child}/include)
	ENDIF()
ENDFOREACH()

if (WITH_UDEV)
    set(dashprint_sources ${dashprint_sources} src/UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)

set(LINK_LIBRARIES ${Boost_LIBRARIES} ${UDEV_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
if(UNIX AND NOT APPLE)
    set(LINK_LIBRARIES ${LINK_LIBRARIES} rt)
endif()

configure_file("${CMAKE_SOURCE_DIR}/dashprint_config.h.in" "${CMAKE_BINARY_DIR}/dashprint_config.h")
include_directories("${CMAKE_BINARY_DIR}")

add_library(boost OBJECT ${boost_sources})

add_executable(dashprint ${dashprint_sources} $<TARGET_OBJECTS:boost>)
target_link_libraries(dashprint ${LINK_LIBRARIES})

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/dashprint/web")
install(TARGETS dashprint DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")


if (WITH_TESTS)
    enable_testing()

    include_directories(${CMAKE_SOURCE_DIR}/src)
    add_executable(PrinterTest test/PrinterTest.cpp src/Printer.cpp $<TARGET_OBJECTS:boost>)
    target_link_libraries(PrinterTest ${LINK_LIBRARIES})

    add_test(PrinterTest PrinterTest)

endif(WITH_TESTS)
