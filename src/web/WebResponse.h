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
	using http_header = boost::beast::http::field;

	void set(boost::beast::http::field hdr, std::string_view value);
	
	void send(http_status status);
	void send(http_status status, const std::map<std::string,std::string>& headers);
	// void send(const std::string& text, std::string_view contentType, http_status status = http_status::ok);
	void send(std::string_view data, std::string_view contentType, http_status status = http_status::ok);
	void send(const nlohmann::json& json, http_status status = http_status::ok);
	void sendFile(const char* path, http_status status = http_status::ok);

	bool sendChunked(http_status status, const std::map<std::string,std::string>& headers = std::map<std::string,std::string>());
	bool sendChunk(const void* data, size_t length);
	void sendFinalChunk();

	static std::string_view mimeType(std::string_view path);
private:
	std::shared_ptr<WebSession> m_session;
	std::map<boost::beast::http::field, std::string_view> m_headers;
};

#endif
