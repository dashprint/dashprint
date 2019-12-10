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

// Resource:
// https://github.com/tasanakorn/rpi-mmal-demo/blob/master/video_record.c

static const int MMAL_CAMERA_VIDEO_PORT = 1;

MMALCamera::MMALCamera()
{
	MMAL_STATUS_T status;

	status = ::mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &m_camera);
	if (status != MMAL_SUCCESS)
	{
		std::stringstream ss;
		ss << "Cannot create camera component: " << status;
		throw std::runtime_error(ss.str());
	}

	setupCameraFormat();
	setupEncoderFormat();
	setupConnection();

	// m_dump.open("/tmp/dumpc.264", std::ios_base::binary);
}


MMALCamera::~MMALCamera()
{
	BOOST_LOG_TRIVIAL(debug) << "MMALCamera destroying";

	if (m_connection)
		::mmal_connection_destroy(m_connection);
	if (m_camera)
		::mmal_component_destroy(m_camera);
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

void MMALCamera::fillEncoderFormat(MMAL_ES_FORMAT_T* fmt)
{
	MMAL_PORT_T* camVideoPort = m_camera->output[MMAL_CAMERA_VIDEO_PORT];

	::mmal_format_copy(fmt, camVideoPort->format);
}

// Connect the output from the camera into encoder's input
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

std::map<std::string, DetectedCamera> MMALCamera::detectCameras()
{
	static int lastDetectionResult = -1;

	std::map<std::string, DetectedCamera> rv;
	if (lastDetectionResult == -1)
	{
		VCHI_INSTANCE_T vchi;
		VCHI_CONNECTION_T* vchi_connections;

		::bcm_host_init();
		::vcos_init();

		lastDetectionResult = 0;

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

		lastDetectionResult = 1;
	}
	else if (lastDetectionResult == 0)
		return rv;
	
	DetectedCamera cam;
	cam.id = "rpicam";
	cam.name = "Raspberry Pi camera";
	cam.instantiate = []() -> std::shared_ptr<Camera> { return std::make_shared<MMALCamera>(); };

	CameraParameter param;
	param.id = "bitrate";
	param.description = "Bitrate";
	param.parameterType = CameraParameter::ParameterType::Integer;
	param.minIntegerValue = 200000; // 200 kbit/s
	param.maxIntegerValue = 25000000; // 25 Mbit/s
	param.defaultIntegerValue = 2000000; // 2 Mbit/s
	cam.parameters.emplace_back(std::move(param));

	rv.emplace(std::make_pair(cam.id, std::move(cam)));
	return rv;
}
