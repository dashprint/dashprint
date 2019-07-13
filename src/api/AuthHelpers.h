#ifndef _AUTHHELPERS_H
#define _AUTHHELPERS_H
#include "web/WebRequest.h"
#include "web/WebResponse.h"

template <typename Callable, typename... Args>
WebRouter::handler_t checkToken(AuthManager* authManager, Callable c, Args... args)
{
	return [=](WebRequest& req, WebResponse& resp) {
		std::string_view hdr = req.header(boost::beast::http::field::www_authenticate);
		if (!hdr.empty())
		{
			if (hdr.compare(0, 7, "Bearer "))
				hdr = hdr.substr(7);
			
			std::string user = authManager->checkToken(std::string(hdr).c_str());
			if (!user.empty())
			{
				req.privateData()["username"] = user;

				c(req, resp, args...);
				return;
			}
		}

		resp.send(boost::beast::http::status::unauthorized);
	};
}

template <typename Callable, typename... Args>
WebRouter::handler_t checkOctoprintKey(AuthManager* authManager, Callable c, Args... args)
{
	return [=](WebRequest& req, WebResponse& resp) {
		std::string_view hdr = req.request()["X-API-Key"];

		if (!hdr.empty())
		{
			std::string user = authManager->authenticateOctoprintCompat(std::string(hdr).c_str());
			if (!user.empty())
			{
				req.privateData()["username"] = user;

				c(req, resp, args...);
				return;
			}
		}

		resp.send(boost::beast::http::status::unauthorized);
	};
}

#endif
