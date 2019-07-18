#ifndef _CAMERAMANAGER_H
#define _CAMERAMANAGER_H
#include <mutex>
#include <map>
#include <string>
#include <memory>
#include <string_view>
#include <boost/property_tree/ptree.hpp>
#include "camera/Camera.h"

class MP4Streamer;
class MP4Sink;

class CameraManager
{
public:
	CameraManager(boost::property_tree::ptree& config);

	// The caller is responsible for deleting the returned instance
	// and for calling releaseStreamer().
	MP4Streamer* createStreamer(std::string_view id, MP4Sink* target);
	void releaseStreamer(std::string_view id);

	union CameraParameterValue
	{
		int integerValue;
	};

	void saveCameraParameters(std::string_view cameraId, const std::map<std::string, CameraParameterValue>& values);
	std::map<std::string, CameraParameterValue> cameraParameters(std::string_view cameraId) const;
private:
	boost::property_tree::ptree& m_config;
	std::mutex m_mutex;

	struct RunningCamera
	{
		size_t refCount = 1;
		std::shared_ptr<Camera> camera;
	};
	std::map<std::string, RunningCamera, std::less<>> m_runningCameras;
};

#endif
