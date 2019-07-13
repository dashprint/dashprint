#include "AuthApi.h"
#include "nlohmann/json.hpp"
#include "web/WebRequest.h"
#include "web/WebResponse.h"
#include "AuthHelpers.h"

namespace
{
	void restAuthLogin(WebRequest& req, WebResponse& resp, AuthManager* authManager)
	{
		nlohmann::json jreq = req.jsonRequest();

		auto username = jreq["username"].get<std::string>();
		auto password = jreq["password"].get<std::string>();

		if (authManager->authenticate(username.c_str(), password.c_str()))
		{
			// Generate and send a JWT
			std::string jwt = authManager->generateToken(username.c_str());
			nlohmann::json rv = {
				{ "token", jwt }
			};

			resp.send(rv);
		}
		else
		{
			resp.send(boost::beast::http::status::unauthorized);
		}
	}

	void restRefreshToken(WebRequest& req, WebResponse& resp, AuthManager* authManager)
	{
		const std::string& username = req.privateData()["username"];

		std::string jwt = authManager->generateToken(username.c_str());
		nlohmann::json rv = {
			{ "token", jwt }
		};

		resp.send(rv);
	}
}

void routeAuth(WebRouter* router, AuthManager& authManager)
{
	router->post("auth/login", restAuthLogin, &authManager);
	router->post("auth/refreshToken", WebRouter::inlineFilter(checkToken(&authManager), restRefreshToken, &authManager));
}

