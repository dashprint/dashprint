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

void WebResponse::send(const std::string& text, const char* contentType, http_status status)
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

void WebResponse::send(std::string_view data, const char* contentType, http_status status)
{
	boost::beast::http::response<boost::beast::http::span_body<char>> res{status, m_session->m_request.version()};

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
