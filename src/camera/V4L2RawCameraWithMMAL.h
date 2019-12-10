#ifndef _V4L2RAWCAMERAWITHMMAL_H
#define _V4L2RAWCAMERAWITHMMAL_H
#include "V4L2GenericCamera.h"
#include "MMALEncoder.h"
#include <memory>
#include <thread>

// A camera capable of providing uncompressed video (typically YUYV)
class V4L2RawCameraWithMMAL : public MMALEncoder, private V4L2GenericCamera
{
public:
	V4L2RawCameraWithMMAL(const char* devicePath);
	~V4L2RawCameraWithMMAL();

	void start() override;
	void stop() override;

	static std::map<std::string, DetectedCamera> detectCameras();
protected:
	void processFrames(const void* data, size_t length) override;
	void fillEncoderFormat(MMAL_ES_FORMAT_T* fmt) override;
private:
	MMAL_POOL_T* m_encoderInputPool = nullptr;
	std::unique_ptr<std::thread> m_v4l2Thread;
};

#endif
