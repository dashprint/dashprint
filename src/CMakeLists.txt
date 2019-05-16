project(dashprint-src)

add_executable(binfile binfile/binfile.cpp)
target_link_libraries(binfile Boost::iostreams Boost::filesystem ${ZLIB_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp"
	COMMAND "${CMAKE_CURRENT_BINARY_DIR}/binfile" "${CMAKE_SOURCE_DIR}/web/dist" "${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp"
    DEPENDS binfile
    COMMENT "Compressing web resources..."
)

# Detect 'mmal_core mmal_util mmal_vc_client vcos bcm_host' for RPi hi-perf camera


include_directories(${CMAKE_SOURCE_DIR}/src)

set(dashprint_sources
	main.cpp
    PrinterDiscovery.cpp
    StaticPrinterDiscovery.cpp
    web/WebServer.cpp
    web/WebSession.cpp
    web/WebRequest.cpp
    web/WebResponse.cpp
    web/WebRouter.cpp
    web/MultipartFormData.cpp
    web/WebSocketHandler.cpp
    api/RestApi.cpp
    api/OctoprintRestApi.cpp
    api/WebSockets.cpp
    Printer.cpp
    PrinterManager.cpp
    util.cpp
    PrintJob.cpp
    PrintJob.h
    FileManager.cpp
	${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp
)

if (WITH_UDEV)
    set(dashprint_sources ${dashprint_sources} UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)


add_executable(dashprint ${dashprint_sources})
target_link_libraries(dashprint ${LINK_LIBRARIES})

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/dashprint/web")
install(TARGETS dashprint DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

#add_subdirectory(wasm)

