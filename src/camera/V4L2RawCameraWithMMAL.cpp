#include "V4L2RawCameraWithMMAL.h"
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <stdexcept>
#include <sstream>

// Capture example: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html

V4L2RawCameraWithMMAL::V4L2RawCameraWithMMAL(const char* devicePath)
: V4L2GenericCamera(devicePath)
{
	std::vector<v4l2_fmtdesc> formats = getFormats();
	const v4l2_fmtdesc* chosenFormat = nullptr;

	// Find suitable format
	for (const auto& fmt : formats)
	{
		if (fmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE && !(fmt.flags & V4L2_FMT_FLAG_COMPRESSED))
		{
			chosenFormat = &fmt;
			break;
		}
	}

	if (!chosenFormat)
		throw std::runtime_error("Couldn't find suitable camera format");

	initDevice(chosenFormat->pixelformat);

	setupEncoderFormat();

	MMAL_STATUS_T status;
	MMAL_PORT_T* encoderInputPort = m_encoder->input[0];
	m_encoderInputPool = ::mmal_port_pool_create(encoderInputPort, encoderInputPort->buffer_num, encoderInputPort->buffer_size);
}

V4L2RawCameraWithMMAL::~V4L2RawCameraWithMMAL()
{
	if (m_encoderInputPool)
	{
		::mmal_port_pool_destroy(m_encoder->input[0], m_encoderInputPool);
	}
}

std::map<std::string, DetectedCamera> V4L2RawCameraWithMMAL::detectCameras()
{
	return V4L2GenericCamera::detectCameras(CameraType::RawCamera);
}

void V4L2RawCameraWithMMAL::start()
{
	MMALEncoder::start();
	V4L2GenericCamera::start();

	m_v4l2Thread.reset(new std::thread([=]{
		V4L2GenericCamera::run();
	}));
}

void V4L2RawCameraWithMMAL::stop()
{
	V4L2GenericCamera::stop();

	if (m_v4l2Thread)
	{
		m_v4l2Thread->join();
		m_v4l2Thread.reset();
	}

	MMALEncoder::stop();
}

void V4L2RawCameraWithMMAL::processFrames(const void* data, size_t length)
{
	MMAL_PORT_T* encoderInputPort = m_encoder->input[0];
	size_t done = 0;

	while (done < length)
	{
		MMAL_BUFFER_HEADER_T* buffer = ::mmal_queue_get(m_encoderInputPool->queue);
		if (!buffer)
			throw std::runtime_error("Cannot get buffer from input pool queue");

		buffer->length = std::min(length - done, buffer->alloc_size);
		memcpy(buffer->data, static_cast<const char*>(data) + done, buffer->length);
		
		MMAL_STATUS_T status = ::mmal_port_send_buffer(encoderInputPort, buffer);
		if (status != MMAL_SUCCESS)
		{
			std::stringstream ss;
			ss << "Cannot send buffer to encoder input port: " << status;
			throw std::runtime_error(ss.str());
		}

		done += buffer->length;
	}
}

void V4L2RawCameraWithMMAL::fillEncoderFormat(MMAL_ES_FORMAT_T* fmt)
{
	v4l2_streamparm params = getParameters();

	fmt->es->video.frame_rate.num = params.parm.capture.timeperframe.numerator;
	fmt->es->video.frame_rate.den = params.parm.capture.timeperframe.denominator;

	v4l2_format v4lFormat = getFormat();

	switch (v4lFormat.fmt.pix.pixelformat)
	{
		case V4L2_PIX_FMT_YUV420:
			fmt->encoding = MMAL_ENCODING_I420;
			break;
		case V4L2_PIX_FMT_YVU420:
			fmt->encoding = MMAL_ENCODING_YV12;
			break;
		case V4L2_PIX_FMT_YUYV:
			fmt->encoding = MMAL_ENCODING_YUYV;
			break;
		case V4L2_PIX_FMT_UYVY:
			fmt->encoding = MMAL_ENCODING_UYVY;
			break;
		case V4L2_PIX_FMT_NV12:
			fmt->encoding = MMAL_ENCODING_NV12;
			break;
		case V4L2_PIX_FMT_NV21:
			fmt->encoding = MMAL_ENCODING_NV21;
			break;
		default:
		{
			std::stringstream ss;
			ss << "Unsupported pixel format 0x" << std::hex << v4lFormat.fmt.pix.pixelformat;
			throw std::runtime_error(ss.str());
		}
	}

    fmt->encoding_variant = fmt->encoding;

    fmt->es->video.width = v4lFormat.fmt.pix.width;
    fmt->es->video.height = v4lFormat.fmt.pix.height;
    fmt->es->video.crop.x = 0;
    fmt->es->video.crop.y = 0;
    fmt->es->video.crop.width = v4lFormat.fmt.pix.width;
    fmt->es->video.crop.height = v4lFormat.fmt.pix.height;
}
