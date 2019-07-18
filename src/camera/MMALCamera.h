#ifndef _MMALCAMERA_H
#define _MMALCAMERA_H
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_connection.h>
#include <string_view>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <list>
#include <stdint.h>
#include "Camera.h"

class H264Source;

class MMALCamera : public Camera, public std::enable_shared_from_this<MMALCamera>
{
public:
	MMALCamera();
	~MMALCamera();

	void start() override;
	void stop() override;
	bool isRunning() const override { return m_running; }
	H264Source* createSource() override;

	static std::map<std::string, DetectedCamera> detectCameras();
private:
	void setupCameraFormat();
	void setupEncoderFormat();
	void setupConnection();
	void encoderCallback(MMAL_BUFFER_HEADER_T* buffer);
	void processBuffer(std::basic_string_view<uint8_t> buffer);
	void processNAL(std::basic_string_view<uint8_t> buffer);

	static void fillPortBuffer(MMAL_PORT_T* port, MMAL_POOL_T* pool);
	static void staticInitialization();
private:
	MMAL_COMPONENT_T* m_camera = nullptr;
	MMAL_COMPONENT_T* m_encoder = nullptr;
	MMAL_POOL_T* m_encoderOutputPool = nullptr;
	MMAL_CONNECTION_T* m_connection = nullptr;
	bool m_running = false;
protected:
	std::vector<uint8_t> m_buffers[20];
	int m_nextBuffer = 0;
	std::vector<uint8_t> m_sps, m_pps;
	std::condition_variable m_cv;
	std::mutex m_cvMutex;
	// std::ofstream m_dump;

	friend class MMALH264Source;
};

#endif
