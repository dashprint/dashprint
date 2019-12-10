#ifndef _MMALCAMERA_H
#define _MMALCAMERA_H
#include <fstream>
#include <list>
#include <stdint.h>
#include "Camera.h"
#include "MMALEncoder.h"

class H264Source;

class MMALCamera : public MMALEncoder
{
public:
	MMALCamera();
	~MMALCamera();

	static std::map<std::string, DetectedCamera> detectCameras();
protected:
	void fillEncoderFormat(MMAL_ES_FORMAT_T* fmt) override;
private:
	void setupCameraFormat();
	void setupConnection();
private:
	MMAL_COMPONENT_T* m_camera = nullptr;
	MMAL_CONNECTION_T* m_connection = nullptr;
	
protected:
	
	// std::ofstream m_dump;

	
};

#endif
