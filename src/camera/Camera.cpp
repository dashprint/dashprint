#include "Camera.h"
#ifdef WITH_MMAL_CAMERA
#	include "MMALCamera.h"
#endif

std::map<std::string, DetectedCamera, std::less<>> Camera::detectCameras()
{
	std::map<std::string, DetectedCamera, std::less<>> cameras;

#ifdef WITH_MMAL_CAMERA
	std::map<std::string, DetectedCamera> newCameras = MMALCamera::detectCameras();
	cameras.insert(newCameras.begin(), newCameras.end());
#endif

	return cameras;
}
