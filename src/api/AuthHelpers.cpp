#include "AuthHelpers.h"
#include "web/WebRequest.h"
#include "web/WebResponse.h"

WebRouter::filter_t checkToken(AuthManager* authManager)
{
	return [=](WebRequest& req, WebResponse& resp, WebRouter::handler_t next) {
		std::string_view hdr = req.header(boost::beast::http::field::www_authenticate);
		if (!hdr.empty())
		{
			if (hdr.compare(0, 7, "Bearer "))
				hdr = hdr.substr(7);
			
			std::string user = authManager->checkToken(std::string(hdr).c_str());
			if (!user.empty())
			{
				req.privateData()["username"] = user;

				next(req, resp);
				return;
			}
		}

		resp.send(boost::beast::http::status::unauthorized);
	};
}

WebRouter::filter_t checkOctoprintKey(AuthManager* authManager)
{
	return [=](WebRequest& req, WebResponse& resp, WebRouter::handler_t next) {
		std::string_view hdr = req.request()["X-API-Key"];

		if (!hdr.empty())
		{
			std::string user = authManager->authenticateOctoprintCompat(std::string(hdr).c_str());
			if (!user.empty())
			{
				req.privateData()["username"] = user;

				next(req, resp);
				return;
			}
		}

		resp.send(boost::beast::http::status::unauthorized);
	};
}
