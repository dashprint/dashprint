#ifndef _V4L2RAWCAMERA_H
#define _V4L2RAWCAMERA_H
#include "V4L2GenericCamera.h"

// A camera capable of providing uncompressed video (typically YUYV)
class V4L2RawCamera : public Camera, private V4L2GenericCamera
{
public:
	V4L2RawCamera(const char* devicePath);

	static std::map<std::string, DetectedCamera> detectCameras();
private:
	int m_fd;
};

#endif
