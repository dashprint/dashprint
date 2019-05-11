#ifndef _WEBRESPONSE_H
#define _WEBRESPONSE_H
#include <boost/beast.hpp>
#include <map>
#include <string>
#include <memory>
#include "nlohmann/json.hpp"
#include "WebSession.h"

class WebResponse
{
public:
	WebResponse(std::shared_ptr<WebSession> session);
	using http_status = boost::beast::http::status;

	void set(boost::beast::http::field hdr, std::string_view value);
	
	void send(http_status status);
	void send(http_status status, const std::map<std::string,std::string>& headers);
	void send(const std::string& text, const char* contentType, http_status status = http_status::ok);
	void send(std::string_view data, const char* contentType, http_status status = http_status::ok);
	void send(const nlohmann::json& json, http_status status = http_status::ok);
	void sendFile(const char* path, http_status status = http_status::ok);
private:
	std::shared_ptr<WebSession> m_session;
	std::map<boost::beast::http::field, std::string_view> m_headers;
};

#endif
