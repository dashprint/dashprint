#include "WebRequest.h"
#include "WebSession.h"
#include "WebServer.h"
#include "MultipartFormData.h"
#include <charconv>

WebRequest::WebRequest(const boost::beast::http::request<boost::beast::http::string_body>& request,
const std::string& requestFile,
std::shared_ptr<WebSession> webSession)
: m_request(request), m_requestFile(requestFile), m_webSession(webSession)
{
	m_target = request.target();

	auto p = m_target.find('?');
	if (p != std::string::npos)
	{
		parseQueryString(m_target.substr(p+1));
		m_target.resize(p);
	}
}

boost::asio::ip::tcp::socket&& WebRequest::socket()
{
	return m_webSession->moveSocket();
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

void WebRequest::parseQueryString(const std::string& qs)
{
	m_queryString = qs;

	ssize_t pos = 0;

	while (pos < qs.length())
	{
		ssize_t newPos = qs.find(pos, '&');
		if (newPos == std::string::npos)
		{
			parseQueryStringKV(std::string_view(qs).substr(pos));
			break;
		}
		else
		{
			parseQueryStringKV(std::string_view(qs).substr(pos, newPos-pos));
			pos = newPos + 1;
		}
	}
}

void WebRequest::parseQueryStringKV(std::string_view sv)
{
	ssize_t pos = sv.find('=');
	if (pos == std::string::npos)
		return;

	std::string key = urlDecode(sv.substr(0, pos));
	std::string value = urlDecode(sv.substr(pos+1));

	m_queryParams.emplace(std::make_pair(std::move(key), std::move(value)));
}

std::string WebRequest::urlDecode(std::string_view in)
{
	std::string out;
	out.reserve(in.size());

	for (std::size_t i = 0; i < in.size(); ++i)
	{
		if (in[i] == '%')
		{
			if (i + 3 <= in.size())
			{
				int value;
				if (auto [p, ec] = std::from_chars(in.data() + i + 1, in.data() + i + 3, value, 16); ec == std::errc())
				{
					out += static_cast<char>(value);
					i += 2;
				}
				else
					return std::string(in);
			}
			else
				return std::string(in);
		}
		else if (in[i] == '+')
		{
			out += ' ';
		}
		else
		{
			out += in[i];
		}
	}
	return out;
}

const char* WebRequest::queryParam(const char* name)
{
	auto it = m_queryParams.find(name);
	if (it != m_queryParams.end())
		return it->second.c_str();
	return nullptr;
}
