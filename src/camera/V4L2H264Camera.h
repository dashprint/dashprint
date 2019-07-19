#ifndef _V4L2H264CAMERA_H
#define _V4L2H264CAMERA_H
#include "Camera.h"

class V4L2H264Camera : public Camera
{
public:
	V4L2H264Camera(const char* devicePath);
};

#endif
