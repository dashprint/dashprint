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
#include <unistd.h>

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
		::mmal_port_disable(m_encoder->output[0]);
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

	encoderOutputPort->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>(this);

	status = ::mmal_port_enable(encoderOutputPort, [](MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
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
	MMAL_PORT_T* encoderOutputPort = m_encoder->output[0];

	::mmal_buffer_header_mem_lock(buffer);

	// Process H264 data
	std::basic_string_view<uint8_t> sv(buffer->data, buffer->length);
	// TODO

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
