#include "MMALCamera.h"
#include <mutex>
#include <bcm_host.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <boost/log/trivial.hpp>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <chrono>
#include <unistd.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include "../mp4/MP4Streamer.h"

// Resource:
// https://github.com/tasanakorn/rpi-mmal-demo/blob/master/video_record.c

static const int MMAL_CAMERA_VIDEO_PORT = 1;

MMALCamera::MMALCamera()
{
	static std::once_flag bcmInitialization;

	std::call_once(bcmInitialization, staticInitialization);

	MMAL_STATUS_T status;

	status = ::mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &m_camera);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Cannot create camera component: " << status;
		throw std::runtime_error(ss.str());
	}

	status = ::mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &m_encoder);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Cannot create encoder component: " << status;
		throw std::runtime_error(ss.str());
	}

	setupCameraFormat();
	setupEncoderFormat();
	setupConnection();

	for (auto& b : m_buffers)
		b.reserve(8192*4);

	m_sps.reserve(30);
	m_pps.reserve(20);
	// m_dump.open("/tmp/dumpc.264", std::ios_base::binary);
}

void MMALCamera::staticInitialization()
{
	if (::access("/dev/vchiq", R_OK | W_OK) != 0)
		throw std::runtime_error("Cannot access /dev/vchiq, add the current user to the video group");
	
	::bcm_host_init();
}

MMALCamera::~MMALCamera()
{
	if (m_encoderOutputPool)
	{
		stop();
		::mmal_port_pool_destroy(m_encoder->output[0], m_encoderOutputPool);
	}
	if (m_connection)
		::mmal_connection_destroy(m_connection);
	if (m_camera)
		::mmal_component_destroy(m_camera);
	if (m_encoder)
		::mmal_component_destroy(m_encoder);
}

void MMALCamera::setupCameraFormat()
{
	MMAL_PORT_T* camVideoPort = m_camera->output[MMAL_CAMERA_VIDEO_PORT];
	MMAL_ES_FORMAT_T* format = camVideoPort->format;

    format->encoding = MMAL_ENCODING_I420;
    format->encoding_variant = MMAL_ENCODING_I420;
    format->es->video.width = 1920;
    format->es->video.height = 1080;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = 1920;
    format->es->video.crop.height = 1080;
    format->es->video.frame_rate.num = 25;
    format->es->video.frame_rate.den = 1;

    camVideoPort->buffer_size = format->es->video.width * format->es->video.height * 12 / 8;
	camVideoPort->buffer_num = 2;

	MMAL_STATUS_T status = ::mmal_port_format_commit(camVideoPort);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Camera format commit failed: " << status;
		throw std::runtime_error(ss.str());
	}

	status = ::mmal_component_enable(m_camera);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Camera enable failed: " << status;
		throw std::runtime_error(ss.str());
	}

	status = ::mmal_port_parameter_set_boolean(camVideoPort, MMAL_PARAMETER_CAPTURE, 1);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "MMAL_PARAMETER_CAPTURE failed: " << status;
		throw std::runtime_error(ss.str());
	}
}

void MMALCamera::setupEncoderFormat()
{
	MMAL_PORT_T* camVideoPort = m_camera->output[MMAL_CAMERA_VIDEO_PORT];
	MMAL_PORT_T* encoderInputPort = m_encoder->input[0];
	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	::mmal_format_copy(encoderInputPort->format, camVideoPort->format);
	encoderInputPort->buffer_size = encoderInputPort->buffer_size_recommended;
	encoderInputPort->buffer_num = 2;

	::mmal_format_copy(encoderOutputPort->format, encoderInputPort->format);

	MMAL_STATUS_T status = ::mmal_port_format_commit(encoderInputPort);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Encoder input format commit failed: " << status;
		throw std::runtime_error(ss.str());
	}

	encoderOutputPort->format->encoding = MMAL_ENCODING_H264;
	encoderOutputPort->format->bitrate = 2000000;

	encoderOutputPort->buffer_size = std::max(encoderOutputPort->buffer_size_recommended, encoderOutputPort->buffer_size_min);
	encoderOutputPort->buffer_num = std::max(2u, encoderOutputPort->buffer_num_min);

	status = ::mmal_port_format_commit(encoderOutputPort);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Encoder output format commit failed: " << status;
		throw std::runtime_error(ss.str());
	}

	m_encoderOutputPool = ::mmal_port_pool_create(encoderOutputPort, encoderOutputPort->buffer_num, encoderOutputPort->buffer_size);
}

void MMALCamera::start()
{
	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	encoderOutputPort->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>(this);

	MMAL_STATUS_T status = ::mmal_port_enable(encoderOutputPort, [](MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
		MMALCamera* This = reinterpret_cast<MMALCamera*>(port->userdata);
		This->encoderCallback(buffer);
	});

	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Encoder output port enabling failed: " << status;
		throw std::runtime_error(ss.str());
	}

	fillPortBuffer(encoderOutputPort, m_encoderOutputPool);

	m_running = true;
}

void MMALCamera::stop()
{
	::mmal_port_disable(m_encoder->output[0]);
	m_running = false;
	m_cv.notify_all();
}

void MMALCamera::setupConnection()
{
	MMAL_PORT_T* camVideoPort = m_camera->output[MMAL_CAMERA_VIDEO_PORT];
	MMAL_PORT_T* encoderInputPort = m_encoder->input[0];

	MMAL_STATUS_T status = ::mmal_connection_create(&m_connection, encoderInputPort, camVideoPort,
		MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
	
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Camera/encoder connection setup failed: " << status;
		throw std::runtime_error(ss.str());
	}

	status = ::mmal_connection_enable(m_connection);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Camera/encoder connection enabling failed: " << status;
		throw std::runtime_error(ss.str());
	}
}

void MMALCamera::encoderCallback(MMAL_BUFFER_HEADER_T* buffer)
{
	std::cout << "Encoder callback\n";
	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	::mmal_buffer_header_mem_lock(buffer);

	// Process H264 data
	std::basic_string_view<uint8_t> sv(buffer->data, buffer->length);
	processBuffer(sv);

	::mmal_buffer_header_mem_unlock(buffer);

	::mmal_buffer_header_release(buffer);

	if (encoderOutputPort->is_enabled)
	{
		MMAL_STATUS_T status;
		MMAL_BUFFER_HEADER_T* newBuffer = ::mmal_queue_get(m_encoderOutputPool->queue);

		if (newBuffer != nullptr)
			status = ::mmal_port_send_buffer(encoderOutputPort, newBuffer);

		if (!newBuffer || status != MMAL_SUCCESS)
			BOOST_LOG_TRIVIAL(warning) << "Unable to return a buffer to the video port";
	}
}

void MMALCamera::fillPortBuffer(MMAL_PORT_T* port, MMAL_POOL_T* pool)
{
	const int count = ::mmal_queue_length(pool->queue);

	for (int i = 0; i < count; i++)
	{
		MMAL_BUFFER_HEADER_T* buffer = ::mmal_queue_get(pool->queue);
		if (!buffer)
			throw std::runtime_error("Cannot get buffer from pool queue");
		
		MMAL_STATUS_T status = ::mmal_port_send_buffer(port, buffer);
		if (status != MMAL_SUCCESS)
		{
			std::stringstream ss;
			ss << "Cannot send buffer to port: " << status;
			throw std::runtime_error(ss.str());
		}
	}
}

void MMALCamera::processBuffer(std::basic_string_view<uint8_t> buffer)
{
	// m_dump.write((char*)buffer.data(), buffer.size());

	if (buffer.length() < 5)
		return;
	
	// Expect an Annex-B start sequence
	if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0 || buffer[3] != 1)
		return;
	
	size_t start = 0;
	// Split into NAL units
	for (size_t i = 1; i < buffer.size()-3; i++)
	{
		if (buffer[i] == 0 && buffer[i+1] == 0 && buffer[i+2] == 0 && buffer[i+3] == 1)
		{
			// This is the start of another NAL
			auto sv = buffer.substr(start, i-start);
			start = i;
			processNAL(sv);
		}
	}
	auto sv = buffer.substr(start);
	processNAL(sv);
}

void MMALCamera::processNAL(std::basic_string_view<uint8_t> buffer)
{
	if (buffer[4] == 0x27)
	{
		std::unique_lock<std::mutex> l(m_cvMutex);

		std::cout << "SPS from camera\n";
		// SPS
		m_sps.assign(buffer.data(), buffer.data() + buffer.length());
	}
	else if (buffer[4] == 0x28)
	{
		std::unique_lock<std::mutex> l(m_cvMutex);
		// PPS
		m_pps.assign(buffer.data(), buffer.data() + buffer.length());
	}
	else
	{
		std::unique_lock<std::mutex> l(m_cvMutex);

		// Enqueue this as a standard NAL
		std::cout << "Writing " << buffer.length() << " into buf " << m_nextBuffer << std::endl;
		m_buffers[m_nextBuffer].assign(buffer.data(), buffer.data() + buffer.length());
		m_nextBuffer = (m_nextBuffer + 1) % std::size(m_buffers);
	}

	m_cv.notify_all();
}

class MMALH264Source : public H264Source
{
public:
	MMALH264Source(std::weak_ptr<MMALCamera> camera, int next)
	: m_camera(camera), m_next(next)
	{
		// m_dump.open("/tmp/dump.264", std::ios_base::binary);
	}
	int read(uint8_t* buf, int bufSize) override
	{
		std::shared_ptr<MMALCamera> camera = m_camera.lock();
		if (!camera || !camera->isRunning())
			return 0;

		if (!m_providedSPS)
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			std::cout << "Waiting for SPS...\n";
			if (camera->m_sps.empty())
			{
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return !camera->m_sps.empty() || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;

			int rd = std::min<int>(camera->m_sps.size(), bufSize);
			std::memcpy(buf, camera->m_sps.data(), rd);
			// m_dump.write((char*) buf, rd);

			m_providedSPS = true;
			return rd;
		}
		else if (!m_providedPPS)
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			if (camera->m_pps.empty())
			{
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return !camera->m_pps.empty() || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;

			int rd = std::min<int>(camera->m_pps.size(), bufSize);
			std::memcpy(buf, camera->m_pps.data(), rd);
			// m_dump.write((char*) buf, rd);

			m_providedPPS = true;
			return rd;
		}
		else
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			if (m_next == camera->m_nextBuffer && m_offset == 0)
			{
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return m_next != camera->m_nextBuffer || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;
			
			int rd = std::min<int>(camera->m_buffers[m_next].size() - m_offset, bufSize);
			std::memcpy(buf, camera->m_buffers[m_next].data() + m_offset, rd);
			// m_dump.write((char*) buf, rd);
			std::cout << "Delivering " << rd << " bytes from buf " << m_next << std::endl;

			if (m_offset + rd >= camera->m_buffers[m_next].size())
			{
				m_next = (m_next + 1) % std::size(camera->m_buffers);
				m_offset = 0;
			}
			else
				m_offset += rd;
			return rd;
		}
	}
private:
	std::weak_ptr<MMALCamera> m_camera;
	bool m_providedSPS = false, m_providedPPS = false;
	int m_next;
	size_t m_offset = 0;
	// std::ofstream m_dump;
};

H264Source* MMALCamera::createSource()
{
	return new MMALH264Source(weak_from_this(), m_nextBuffer);
}

std::list<DetectedCamera> MMALCamera::detectCameras()
{
	std::list<DetectedCamera> rv;
	VCHI_INSTANCE_T vchi;
	VCHI_CONNECTION_T* vchi_connections;

	::bcm_host_init();
	::vcos_init();

	if (::vchi_initialise(&vchi) != 0)
		return rv;
	if (::vchi_connect(nullptr, 0, vchi) != 0)
		return rv;
	::vc_vchi_gencmd_init(vchi, &vchi_connections, 1);

	char response[128];
	int res = ::vc_gencmd(response, sizeof(response), "get_camera");

	::vc_gencmd_stop();
	::vchi_disconnect(vchi);

	if (res != 0)
		return rv;

	int supported, detected;
	if (::sscanf(response, "supported=%d detected=%d", &supported, &detected) != 2)
		return rv;

	if (!supported || !detected)
		return rv;
	
	DetectedCamera cam;
	cam.id = "rpicam";
	cam.name = "Raspberry Pi camera";
	cam.instantiate = []() -> Camera* { return new MMALCamera; };

	CameraParameter param;
	param.id = "bitrate";
	param.description = "Bitrate";
	param.parameterType = CameraParameter::ParameterType::Integer;
	param.minIntegerValue = 200000; // 200 kbit/s
	param.maxIntegerValue = 25000000; // 25 Mbit/s
	param.defaultIntegerValue = 2000000; // 2 Mbit/s
	cam.parameters.emplace_back(std::move(param));

	rv.emplace_back(std::move(cam));
	return rv;
}
