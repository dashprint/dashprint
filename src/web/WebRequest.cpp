#include "WebRequest.h"
#include "WebSession.h"
#include "WebServer.h"
#include "MultipartFormData.h"

boost::asio::ip::tcp::socket&& WebRequest::socket()
{
	return m_webSession->socket();
}

nlohmann::json WebRequest::jsonRequest() const
{
	if (header(boost::beast::http::field::content_type) != "application/json")
		throw WebErrors::not_acceptable("JSON expected");

	return nlohmann::json::parse(m_request.body());
}

std::string WebRequest::baseUrl() const
{
	// TODO: HTTPS
	std::stringstream ss;
	ss << "http://" << host();

	return ss.str();
}

MultipartFormData* WebRequest::multipartBody()
{
	std::string_view contentType = header(boost::beast::http::field::content_type);
	std::string value;
	MultipartFormData::Headers_t params;

	MultipartFormData::parseKV(contentType, value, params);

	if (value != "multipart/form-data")
		return nullptr;
	
	if (params.find("boundary") == params.end())
		return nullptr;
	
	if (hasRequestFile())
		return new MultipartFormData(m_requestFile.c_str(), params["boundary"].c_str());
	else
		return new MultipartFormData(&m_request.body(), params["boundary"].c_str());
}
