#include "Camera.h"
#ifdef WITH_MMAL_CAMERA
#	include "MMALCamera.h"
#endif

std::list<DetectedCamera> Camera::detectCameras()
{
	std::list<DetectedCamera> cameras, newCameras;

#ifdef WITH_MMAL_CAMERA
	newCameras = MMALCamera::detectCameras();
	cameras.insert(cameras.end(), newCameras.begin(), newCameras.end());
#endif

	return cameras;
}
