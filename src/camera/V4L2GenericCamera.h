#ifndef _V4L2GENERICCAMERA_H
#define _V4L2GENERICCAMERA_H
#include "Camera.h"
#include <linux/videodev2.h>
#include <vector>
#include <stdint.h>
#include <memory>
#include <sys/mman.h>

class V4L2GenericCamera
{
protected:
	V4L2GenericCamera(const char* devicePath);
	~V4L2GenericCamera();
protected:
	static void throwErrno(const char* message);
	v4l2_capability getCapability();
	std::vector<v4l2_fmtdesc> getFormats();

	enum class CameraType
	{
		RawCamera,
		H264Camera
	};
	static std::map<std::string, DetectedCamera> detectCameras(CameraType t);
	static bool probeCamera(DetectedCamera& c, const char* path, CameraType t);

	void initDevice(uint32_t pixelFormat);
	void start();
	void stop();
	void run();

	virtual void processFrames(const void* data, size_t length) = 0;
private:
	int m_fd;
	std::unique_ptr<uint8_t[]> m_buffer;
	size_t m_bufferSize;

	struct MappedBuffer
	{
		MappedBuffer(const MappedBuffer&) = delete;
		MappedBuffer& operator=(MappedBuffer const&) = delete;
		MappedBuffer() {}
		MappedBuffer(MappedBuffer&& that)
		{
			address = that.address;
			length = that.length;
			that.address = MAP_FAILED;
		}
		~MappedBuffer()
		{
			if (address != MAP_FAILED)
				::munmap(const_cast<void*>(address), length);
		}
		const void* address = MAP_FAILED;
		size_t length;
	};
	std::vector<MappedBuffer> m_mappedBuffers;
	bool m_usingMmap;
	bool m_stop = false;
};

#endif
