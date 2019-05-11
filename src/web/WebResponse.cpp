#include "WebResponse.h"
#include "WebServer.h"

WebResponse::WebResponse(std::shared_ptr<WebSession> session)
: m_session(session)
{

}

// TODO: send() should invoke doRead() on WebSession
// -> replacement for the loop in doRead()

void WebResponse::send(http_status status, const std::map<std::string,std::string>& headers)
{
	boost::beast::http::response<boost::beast::http::empty_body> res{ status, m_session->m_request.version() };
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

	for (auto it = headers.begin(); it != headers.end(); it++)
		res.set(it->first, it->second);

	res.content_length(0);
	res.keep_alive(m_session->m_request.keep_alive());

	m_session->send(std::move(res));
}

void WebResponse::send(http_status status)
{
	boost::beast::http::response<boost::beast::http::empty_body> res{ status, m_session->m_request.version() };
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

	for (const auto& [k, v] : m_headers)
		res.set(k, v);
        
	res.content_length(0);
	res.keep_alive(m_session->m_request.keep_alive());
	
	m_session->send(std::move(res));
}

/*
void WebResponse::send(const std::string& text, std::string_view contentType, http_status status)
{
	boost::beast::http::response<boost::beast::http::string_body> res{status, m_session->m_request.version()};
	
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(boost::beast::http::field::content_type, contentType);

	for (const auto& [k, v] : m_headers)
		res.set(k, v);
	
	res.keep_alive(m_session->m_request.keep_alive());
	res.body() = text;
	res.prepare_payload();
	
	m_session->send(std::move(res));
}
*/

void WebResponse::sendFile(const char* path, http_status status)
{
	boost::beast::error_code ec;
	boost::beast::http::basic_file_body<boost::beast::file_posix>::value_type body;

	body.open(path, boost::beast::file_mode::scan, ec);
	if (!body.is_open())
		throw WebErrors::not_found("Cannot open file");

	auto size = body.size();

	boost::beast::http::response<boost::beast::http::file_body> res{
				std::piecewise_construct,
				std::make_tuple(std::move(body)),
				std::make_tuple(boost::beast::http::status::ok, m_session->m_request.version())};
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

	for (const auto& [k, v] : m_headers)
		res.set(k, v);

	res.content_length(size);
	res.keep_alive(m_session->m_request.keep_alive());

	m_session->send(std::move(res));
}

void WebResponse::send(const nlohmann::json& json, http_status status)
{
	send(json.dump(), "application/json", status);
}

void WebResponse::send(std::string_view data, std::string_view contentType, http_status status)
{
	boost::beast::http::response<boost::beast::http::span_body<char const>> res{status, m_session->m_request.version()};

	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(boost::beast::http::field::content_type, contentType);

	for (const auto& [k, v] : m_headers)
		res.set(k, v);
	
	res.keep_alive(m_session->m_request.keep_alive());
	res.body() = data;
	res.prepare_payload();
	
	m_session->send(std::move(res));
}

void WebResponse::set(boost::beast::http::field hdr, std::string_view value)
{
	m_headers.insert({hdr, value});
}

std::string_view WebResponse::mimeType(std::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == std::string_view::npos)
            return std::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
	if(iequals(ext, ".xhtml")) return "application/xhtml+xml";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}