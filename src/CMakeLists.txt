project(dashprint-src)

add_subdirectory(binfile)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp"
	COMMAND "${BINFILE_EXE}" "${CMAKE_SOURCE_DIR}/web/dist" "${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp"
    DEPENDS binfileexe
    COMMENT "Compressing web resources..."
)

# Detect 'mmal_core mmal_util mmal_vc_client vcos bcm_host' for RPi hi-perf camera

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/bcrypt
    ${CMAKE_SOURCE_DIR}/src/cpp-jwt/include
)

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
    api/PrintApi.cpp
    api/OctoprintRestApi.cpp
    api/WebSockets.cpp
    api/AuthApi.cpp
    api/AuthHelpers.cpp
    api/CameraApi.cpp
    Printer.cpp
    PrinterManager.cpp
    util.cpp
    PrintJob.cpp
    PrintJob.h
    FileManager.cpp
    AuthManager.cpp
    bcrypt/bcrypt.c
    bcrypt/blowfish.c
    PluginManager.cpp
    Plugin.cpp
    mp4/MP4Streamer.cpp
    camera/Camera.cpp
    CameraManager.cpp
	${CMAKE_CURRENT_BINARY_DIR}/webdata.cpp
)

if (WITH_UDEV)
    set(dashprint_sources ${dashprint_sources} UdevPrinterDiscovery.cpp)
endif (WITH_UDEV)

if (BCMHOST_FOUND)
    set(dashprint_sources ${dashprint_sources} camera/MMALCamera.cpp)
endif (BCMHOST_FOUND)

add_executable(dashprint ${dashprint_sources})
target_link_libraries(dashprint ${LINK_LIBRARIES} OpenSSL::SSL quickjs -ldl)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/web/dist/" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/dashprint/web")
install(TARGETS dashprint DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

add_subdirectory(quickjs)
#add_subdirectory(wasm)

