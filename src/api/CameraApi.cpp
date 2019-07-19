#include "CameraApi.h"
#include "../web/WebRequest.h"
#include "../web/WebResponse.h"
#include "../camera/Camera.h"
#include "../nlohmann/json.hpp"
#include "AuthHelpers.h"
#include "mp4/MP4Streamer.h"
#include <boost/log/trivial.hpp>
#include <iostream>
#include <thread>

namespace
{
	void restDetectCameras(WebRequest& req, WebResponse& resp, CameraManager* cameraManager)
	{
		nlohmann::json rv = nlohmann::json::array();
		auto cameras = Camera::detectCameras();

		for (const auto& [id, cam] : cameras)
		{
			nlohmann::json params = nlohmann::json::array();
			std::map<std::string, CameraManager::CameraParameterValue> savedParameters = cameraManager->cameraParameters(id);

			for (const auto& param : cam.parameters)
			{
				nlohmann::json p = {
					{ "id", param.id },
					{ "description", param.description }
				};
				auto itSaved = savedParameters.find(param.id);

				switch (param.parameterType)
				{
					case CameraParameter::ParameterType::Integer:
					{
						p["type"] = "integer";
						p["minIntegerValue"] = param.minIntegerValue;
						p["maxIntegerValue"] = param.maxIntegerValue;
						p["defaultIntegerValue"] = param.defaultIntegerValue;

						if (itSaved != savedParameters.end())
							p["value"] = itSaved->second.integerValue;

						break;
					}
				}

				params.push_back(std::move(p));
			}

			nlohmann::json obj = {
				{ "id", cam.id },
				{ "name", cam.name },
				{ "parameters", params }
			};

			rv.push_back(std::move(obj));
		}

		resp.send(rv);
	}

	class RestMP4Sink : public MP4Sink
	{
	public:
		RestMP4Sink()
		{
		}

		int write(const uint8_t* buf, int bufSize) override
		{
			// BOOST_LOG_TRIVIAL(trace) << "Sending a chunk of " << bufSize << " bytes";
			if (!m_resp->sendChunk(buf, bufSize))
			{
				BOOST_LOG_TRIVIAL(warning) << "Failed to send the chunk";
				m_streamer->stop();
				return 0;
			}
			return bufSize;
		}

		void setStreamer(MP4Streamer* streamer)
		{
			m_streamer = streamer;
		}

		void setWebResponse(WebResponse* webResponse)
		{
			m_resp = webResponse;
		}
	private:
		WebResponse* m_resp;
		MP4Streamer* m_streamer;
	};

	void restStreamCamera(WebRequest& req, WebResponse& resp, CameraManager* cameraManager, AuthManager* authManager)
	{
		// Check token sent as a query param
		const char* token = req.queryParam("token");
		if (!token)
		{
			resp.send(boost::beast::http::status::unauthorized);
			return;
		}

		std::string user = authManager->checkToken(token);

		if (user.empty())
		{
			resp.send(boost::beast::http::status::unauthorized);
			return;
		}

		auto cameraId = req.pathParam(1);
		std::shared_ptr<RestMP4Sink> sink = std::make_shared<RestMP4Sink>();
		std::shared_ptr<MP4Streamer> streamer;

		streamer.reset(cameraManager->createStreamer(cameraId, sink.get()));
		sink->setStreamer(streamer.get());

		if (streamer)
		{
			resp.set(boost::beast::http::field::content_type, "video/mp4");
			if (!resp.sendChunked(boost::beast::http::status::ok))
			{
				cameraManager->releaseStreamer(cameraId);
				return;
			}

			std::thread thread([=]() mutable {
				try
				{
					sink->setWebResponse(&resp);
					streamer->run();
				}
				catch (const std::exception& e)
				{
					BOOST_LOG_TRIVIAL(error) << "Camera streaming error: " << e.what();
				}

				cameraManager->releaseStreamer(cameraId);
				resp.sendFinalChunk();
			});
			thread.detach();
		}
		else
			resp.send(boost::beast::http::status::not_found);
	}
}

void routeCamera(WebRouter* router, CameraManager& cameraManager, AuthManager& authManager)
{
	router->get("detectCameras", WebRouter::inlineFilter(checkToken(&authManager), restDetectCameras, &cameraManager));
	router->get("stream/(.+)", restStreamCamera, &cameraManager, &authManager);
}

