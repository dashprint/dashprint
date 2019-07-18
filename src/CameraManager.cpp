#include "CameraManager.h"
#include <boost/log/trivial.hpp>
#include "mp4/MP4Streamer.h"

CameraManager::CameraManager(boost::property_tree::ptree& config)
: m_config(config)
{

}

MP4Streamer* CameraManager::createStreamer(std::string_view id, MP4Sink* target)
{
	std::lock_guard<std::mutex> l(m_mutex);
	std::shared_ptr<Camera> camera;

	try
	{
		auto it = m_runningCameras.find(id);
		if (it != m_runningCameras.end())
		{
			it->second.refCount++;
			camera = it->second.camera;
		}
		else
		{
			auto cameras = Camera::detectCameras();
			auto it = cameras.find(id);

			if (it == cameras.end())
				return nullptr;
			
			camera = it->second.instantiate();
			if (!camera)
				return nullptr;

			// TODO: set camera parameters

			camera->start();

			RunningCamera rc;
			rc.camera = camera;

			m_runningCameras.emplace(std::string(id), rc);
		}

		std::shared_ptr<H264Source> source(camera->createSource());
		return new MP4Streamer(source, target);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Error creating streamer for camera " << id << ": " << e.what();
		return nullptr;
	}
}

void CameraManager::releaseStreamer(std::string_view id)
{
	std::lock_guard<std::mutex> l(m_mutex);
	auto it = m_runningCameras.find(id);

	if (it != m_runningCameras.end())
	{
		if (!--it->second.refCount)
		{
			it->second.camera->stop();
			m_runningCameras.erase(it);
		}
	}
	else
		BOOST_LOG_TRIVIAL(error) << "CameraManager::releaseStreamer() camera over-released!";
}

void CameraManager::saveCameraParameters(std::string_view cameraId, const std::map<std::string, CameraManager::CameraParameterValue>& values)
{

}

std::map<std::string, CameraManager::CameraParameterValue> CameraManager::cameraParameters(std::string_view cameraId) const
{
	std::map<std::string, CameraManager::CameraParameterValue> rv;
	// TODO
	return rv;
}
