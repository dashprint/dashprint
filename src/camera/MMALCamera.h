#ifndef _MMALCAMERA_H
#define _MMALCAMERA_H
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_connection.h>

class MMALCamera
{
public:
	MMALCamera();
	~MMALCamera();
private:
	void setupCameraFormat();
	void setupEncoderFormat();
	void setupConnection();
	void encoderCallback(MMAL_BUFFER_HEADER_T* buffer);

	static void fillPortBuffer(MMAL_PORT_T* port, MMAL_POOL_T* pool);
	static void staticInitialization();
private:
	MMAL_COMPONENT_T* m_camera = nullptr;
	MMAL_COMPONENT_T* m_encoder = nullptr;
	MMAL_POOL_T* m_encoderOutputPool = nullptr;
	MMAL_CONNECTION_T* m_connection = nullptr;
};

#endif
