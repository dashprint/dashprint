#include "V4L2RawCamera.h"

// Capture example: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html

V4L2RawCamera::V4L2RawCamera(const char* devicePath)
: V4L2GenericCamera(devicePath)
{
}

std::map<std::string, DetectedCamera> V4L2RawCamera::detectCameras()
{
	return V4L2GenericCamera::detectCameras(CameraType::RawCamera);
}
