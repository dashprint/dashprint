#include "V4L2GenericCamera.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
#include <errno.h>
#include <cstring>
#include <sstream>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

V4L2GenericCamera::V4L2GenericCamera(const char* devicePath)
{
	m_fd = ::open(devicePath, O_RDWR | O_NONBLOCK);
	if (m_fd == -1)
		throwErrno("Opening video device failed");
}

V4L2GenericCamera::~V4L2GenericCamera()
{
	if (m_fd != -1)
		::close(m_fd);
}

void V4L2GenericCamera::throwErrno(const char* message)
{
	std::stringstream ss;
	ss << message << ": " << ::strerror(errno);

	throw std::runtime_error(ss.str());
}

v4l2_capability V4L2GenericCamera::getCapability()
{
	v4l2_capability caps;
	if (::ioctl(m_fd, VIDIOC_QUERYCAP, &caps) != 0)
		throwErrno("VIDIOC_QUERYCAP failed");
	return caps;
}

std::vector<v4l2_fmtdesc> V4L2GenericCamera::getFormats()
{
	std::vector<v4l2_fmtdesc> rv;

	for (int i = 0;; i++)
	{
		v4l2_fmtdesc fmt;
		fmt.index = i;

		if (::ioctl(m_fd, VIDIOC_ENUM_FMT, &fmt) == -1)
		{
			if (errno == EINVAL)
				break;
			throwErrno("VIDIOC_ENUM_FMT");
		}

		rv.push_back(fmt);
	}

	return rv;
}

bool V4L2GenericCamera::probeCamera(DetectedCamera& c, const char* path, CameraType t)
{
	V4L2GenericCamera cam(path);

	v4l2_capability caps = cam.getCapability();

	if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) || (caps.capabilities & V4L2_CAP_TUNER))
		return false;
	
	auto formats = cam.getFormats();
	bool foundH264 = false, foundRaw = false;

	for (const auto& fmt : formats)
	{
		if (fmt.type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
			continue;
		
		if (fmt.pixelformat == V4L2_PIX_FMT_H264)
			foundH264 = true;
		else if (!(fmt.flags & V4L2_FMT_FLAG_COMPRESSED))
			foundRaw = true;
	}

	if ((t == CameraType::RawCamera && !foundRaw) || (t == CameraType::H264Camera && !foundH264))
		return false;

	c.name = reinterpret_cast<const char*>(caps.card);

	return true;
}

std::map<std::string, DetectedCamera> V4L2GenericCamera::detectCameras(CameraType t)
{
	std::map<std::string, DetectedCamera> rv;

	for (auto& p : boost::filesystem::directory_iterator("/dev"))
	{
		if (boost::filesystem::is_directory(p))
			continue;
		
		if (boost::starts_with(p.path().filename().generic_string(), "video"))
		{
			DetectedCamera camera;
			
			if (probeCamera(camera, p.path().generic_string().c_str(), t))
				rv.emplace(camera.id, std::move(camera));
		}
	}

	return rv;
}

void V4L2GenericCamera::initDevice(uint32_t pixelFormat)
{
	v4l2_format fmt;
	v4l2_cropcap cropcap = {0};

	v4l2_capability cap = getCapability();

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Reset cropping
	if (::ioctl(m_fd, VIDIOC_CROPCAP, &cropcap) == 0)
	{
		v4l2_crop crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;
		::ioctl(m_fd, VIDIOC_S_CROP, &crop);
	}

	if (::ioctl(m_fd, VIDIOC_G_FMT, &fmt) != 0)
		throwErrno("VIDIOC_G_FMT failed");

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat = pixelFormat;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	if (cap.capabilities & V4L2_CAP_STREAMING)
	{
		v4l2_requestbuffers req;

		req.count = 4;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

		if (::ioctl(m_fd, VIDIOC_REQBUFS, &req) == -1)
			throwErrno("VIDIOC_REQBUFS failed");

		if (req.count < 2)
			throw std::runtime_error("Insufficient buffer memory for camera");

		for (int i = 0; i < req.count; i++)
		{
			v4l2_buffer buf = {0};

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (::ioctl(m_fd, VIDIOC_QUERYBUF, &buf) == -1)
				throwErrno("VIDIOC_QUERYBUF failed");

			MappedBuffer mappedBuffer;

			mappedBuffer.length = buf.length;
			mappedBuffer.address = ::mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, buf.m.offset);

			if (mappedBuffer.address == MAP_FAILED)
				throwErrno("Memory mapping a V4L2 buffer failed");

			m_mappedBuffers.push_back(std::move(mappedBuffer));
		}

		m_usingMmap = true;
	}
	else if (cap.capabilities & V4L2_CAP_READWRITE)
	{
		m_bufferSize = fmt.fmt.pix.sizeimage;
		m_buffer.reset(new uint8_t[m_bufferSize]);
		m_usingMmap = false;
	}
	else
		throw std::runtime_error("No supported video delivery method found");
}

void V4L2GenericCamera::start()
{
	if (m_usingMmap)
	{
		for (size_t i = 0; i < m_mappedBuffers.size(); i++)
		{
			struct v4l2_buffer buf = {0};

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (::ioctl(m_fd, VIDIOC_QBUF, &buf) == -1)
				throwErrno("VIDIOC_QBUF failed");
		}

		v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (::ioctl(m_fd, VIDIOC_STREAMON, &type) == -1)
			throwErrno("VIDIOC_STREAMON failed");
	}
}

void V4L2GenericCamera::stop()
{
	if (m_usingMmap)
	{
		v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (::ioctl(m_fd, VIDIOC_STREAMOFF, &type) == -1)
			throwErrno("VIDIOC_STREAMOFF failed");
	}
}

void V4L2GenericCamera::run()
{
	// TODO
}

