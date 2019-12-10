#include "MMALEncoder.h"
#include <bcm_host.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <mutex>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <boost/log/trivial.hpp>
#include "../mp4/MP4Streamer.h"

MMALEncoder::MMALEncoder()
{
	static std::once_flag bcmInitialization;

	std::call_once(bcmInitialization, staticInitialization);

	MMAL_STATUS_T status;

	status = ::mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_ENCODER, &m_encoder);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Cannot create encoder component: " << status;
		throw std::runtime_error(ss.str());
	}

	for (auto& b : m_buffers)
		b.reserve(8192*4);

	m_sps.reserve(30);
	m_pps.reserve(20);
}

MMALEncoder::~MMALEncoder()
{
	if (m_encoderOutputPool)
	{
		stop();
		::mmal_port_pool_destroy(m_encoder->output[0], m_encoderOutputPool);
	}
	if (m_encoder)
		::mmal_component_destroy(m_encoder);
}

void MMALEncoder::start()
{
	BOOST_LOG_TRIVIAL(debug) << "MMALEncoder starting";

	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	encoderOutputPort->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>(this);

	MMAL_STATUS_T status = ::mmal_port_enable(encoderOutputPort, [](MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
		MMALEncoder* This = reinterpret_cast<MMALEncoder*>(port->userdata);
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

void MMALEncoder::encoderCallback(MMAL_BUFFER_HEADER_T* buffer)
{
	// std::cout << "Encoder callback\n";
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

void MMALEncoder::fillPortBuffer(MMAL_PORT_T* port, MMAL_POOL_T* pool)
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

void MMALEncoder::stop()
{
	BOOST_LOG_TRIVIAL(debug) << "MMALEncoder stopping";

	::mmal_port_disable(m_encoder->output[0]);
	m_running = false;
	m_cv.notify_all();
}

void MMALEncoder::staticInitialization()
{
	if (::access("/dev/vchiq", R_OK | W_OK) != 0)
		throw std::runtime_error("Cannot access /dev/vchiq, add the current user to the video group");
	
	::bcm_host_init();
}

void MMALEncoder::processBuffer(std::basic_string_view<uint8_t> buffer)
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

void MMALEncoder::processNAL(std::basic_string_view<uint8_t> buffer)
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
		// std::cout << "Writing " << buffer.length() << " into buf " << m_nextBuffer << std::endl;
		size_t done = 0;

		// Part of stability workarounds for RPi 1
		while (done < buffer.length())
		{
			size_t amt = std::min<size_t>(8192*12, buffer.length()-done);
			m_buffers[m_nextBuffer].assign(buffer.data()+done, buffer.data() + amt + done);
			m_nextBuffer = (m_nextBuffer + 1) % std::size(m_buffers);
			done += amt;
		}
	}

	m_cv.notify_all();
}

void MMALEncoder::setupEncoderFormat()
{
	MMAL_PORT_T* encoderInputPort = m_encoder->input[0];
	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	fillEncoderFormat(encoderInputPort->format);

	encoderInputPort->buffer_size = encoderInputPort->buffer_size_recommended;
	encoderInputPort->buffer_num = 4;

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

	// Using twice the recommended size fixes corruption on RPi 1
	encoderOutputPort->buffer_size = std::max(encoderOutputPort->buffer_size_recommended*2, encoderOutputPort->buffer_size_min);
	encoderOutputPort->buffer_num = std::max(3u, encoderOutputPort->buffer_num_min);

	status = ::mmal_port_format_commit(encoderOutputPort);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Encoder output format commit failed: " << status;
		throw std::runtime_error(ss.str());
	}

	m_encoderOutputPool = ::mmal_port_pool_create(encoderOutputPort, encoderOutputPort->buffer_num, encoderOutputPort->buffer_size);
}

class MMALH264Source : public H264Source
{
public:
	MMALH264Source(std::weak_ptr<MMALEncoder> camera, int next)
	: m_camera(camera), m_next(next)
	{
		// m_dump.open("/tmp/dump.264", std::ios_base::binary);
	}
	int read(uint8_t* buf, int bufSize) override
	{
		std::shared_ptr<MMALEncoder> camera = m_camera.lock();
		if (!camera || !camera->isRunning())
		{
			if (!camera)
				BOOST_LOG_TRIVIAL(debug) << "MMALH264Source: Camera is gone";
			else
				BOOST_LOG_TRIVIAL(debug) << "MMALH264Source: Camera is stopped";
			return 0;
		}

		if (!m_providedSPS)
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			if (camera->m_sps.empty())
			{
				// std::cout << "Waiting for SPS...\n";
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return !camera->m_sps.empty() || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;

			int rd = std::min<int>(camera->m_sps.size(), bufSize);
			std::memcpy(buf, camera->m_sps.data(), rd);
			// m_dump.write((char*) buf, rd);

			// std::cout << this << "Providing " << rd << " bytes of SPS data\n";
			m_providedSPS = true;
			return rd;
		}
		else if (!m_providedPPS)
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			if (camera->m_pps.empty())
			{
				// std::cout << "Waiting for PPS...\n";
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return !camera->m_pps.empty() || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;

			int rd = std::min<int>(camera->m_pps.size(), bufSize);
			std::memcpy(buf, camera->m_pps.data(), rd);
			// m_dump.write((char*) buf, rd);

			// std::cout << this << "Providing " << rd << " bytes of PPS data\n";
			m_providedPPS = true;
			return rd;
		}
		else
		{
			std::unique_lock<std::mutex> l(camera->m_cvMutex);

			if (m_next == camera->m_nextBuffer && m_offset == 0)
			{
				// std::cout << this << " Waiting for more data...\n";
				if (!camera->m_cv.wait_for(l, std::chrono::seconds(10), [&]() { return m_next != camera->m_nextBuffer || !camera->isRunning(); }))
					return 0;
			}
			if (!camera->isRunning())
				return 0;
			
			int rd = std::min<int>(camera->m_buffers[m_next].size() - m_offset, bufSize);
			std::memcpy(buf, camera->m_buffers[m_next].data() + m_offset, rd);
			// m_dump.write((char*) buf, rd);
			// std::cout << this << " Delivering " << rd << " bytes from buf " << m_next << " of size " << camera->m_buffers[m_next].size() << std::endl;

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
	std::weak_ptr<MMALEncoder> m_camera;
	bool m_providedSPS = false, m_providedPPS = false;
	int m_next;
	size_t m_offset = 0;
	// std::ofstream m_dump;
};

H264Source* MMALEncoder::createSource()
{
	return new MMALH264Source(weak_from_this(), m_nextBuffer);
}
