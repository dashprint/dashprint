#ifndef _MMAPENCODER_H
#define _MMAPENCODER_H
#include <interface/mmal/mmal.h>
#include <interface/mmal/util/mmal_connection.h>
#include "Camera.h"
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <string_view>

class MMALEncoder : public Camera, public std::enable_shared_from_this<MMALEncoder>
{
public:
	MMALEncoder();
	~MMALEncoder();

	H264Source* createSource() override;
	bool isRunning() const override { return m_running; }
	void start() override;
	void stop() override;
private:
	static void staticInitialization();
protected:
	void processBuffer(std::basic_string_view<uint8_t> buffer);
	void processNAL(std::basic_string_view<uint8_t> buffer);
	void setupEncoderFormat();

	virtual void fillEncoderFormat(MMAL_ES_FORMAT_T* fmt) = 0;

	void encoderCallback(MMAL_BUFFER_HEADER_T* buffer);

	static void fillPortBuffer(MMAL_PORT_T* port, MMAL_POOL_T* pool);
protected:
	bool m_running = false;
	MMAL_COMPONENT_T* m_encoder = nullptr;
	MMAL_POOL_T* m_encoderOutputPool = nullptr;

	std::vector<uint8_t> m_buffers[30];
	int m_nextBuffer = 0;
	std::vector<uint8_t> m_sps, m_pps;
	std::condition_variable m_cv;
	std::mutex m_cvMutex;

	friend class MMALH264Source;
};

#endif
