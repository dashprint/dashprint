#ifndef _CAMERA_H
#define _CAMERA_H
#include <functional>
#include <string>
#include <list>
#include <map>
#include <memory>

struct DetectedCamera;
class H264Source;

class Camera
{
public:
	virtual ~Camera() {}
	static std::map<std::string, DetectedCamera, std::less<>> detectCameras();

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual bool isRunning() const = 0;
	virtual H264Source* createSource() = 0;
};

struct CameraParameter
{
	std::string id, description;
	enum class ParameterType
	{
		Integer
	};

	ParameterType parameterType;

	union
	{
		struct
		{
			int minIntegerValue, maxIntegerValue, defaultIntegerValue;
		};
	};
};

struct DetectedCamera
{
	typedef std::function<std::shared_ptr<Camera>()> ctor_t;

	std::string id, name;
	ctor_t instantiate;
	std::list<CameraParameter> parameters;
};

#endif
