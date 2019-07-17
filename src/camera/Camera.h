#ifndef _CAMERA_H
#define _CAMERA_H
#include <functional>
#include <string>
#include <list>

struct DetectedCamera;

class Camera
{
public:
	static std::list<DetectedCamera> detectCameras();
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
	typedef std::function<Camera*()> ctor_t;

	std::string id, name;
	ctor_t instantiate;
	std::list<CameraParameter> parameters;
};

#endif
