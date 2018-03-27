/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebSession.cpp
 * Author: lubos
 * 
 * Created on 27. března 2018, 22:00
 */

#include "WebSession.h"
#include "WebServer.h"
#include "pi3print_config.h"
#include "WebRESTHandler.h"

using namespace std::placeholders;

WebSession::WebSession(WebServer* ws, boost::asio::ip::tcp::socket socket)
: m_server(ws), m_socket(std::move(socket)), m_strand(m_socket.get_executor())
{
}


WebSession::~WebSession()
{
}

void WebSession::run()
{
	doRead();
}

void WebSession::doRead()
{
	m_request = {};
	
	boost::beast::http::async_read(m_socket, m_buffer, m_request,
			boost::asio::bind_executor(m_strand, std::bind(&WebSession::onRead, shared_from_this(), _1, _2)));
}

void WebSession::onRead(boost::system::error_code ec, std::size_t bytesTransferred)
{
	boost::ignore_unused(bytesTransferred);
	
	if(ec == boost::beast::http::error::end_of_stream)
		doClose();
	else if (ec)
		return;
	
	handleRequest();
}

void WebSession::doClose()
{
	boost::system::error_code ec;
	m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
}

static std::string path_cat(boost::beast::string_view base, boost::beast::string_view path)
{
    if(base.empty())
        return path.to_string();
    std::string result = base.to_string();
#if BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// A lot of the following code was taken from
// http://www.boost.org/doc/libs/master/libs/beast/example/http/server/async/http_server_async.cpp


template<bool isRequest, class Body, class Fields>
void WebSession::send(boost::beast::http::message<isRequest, Body, Fields>&& response)
{
	// Ensure that this and response's lifetime extends until async_write() is complete
	auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>>(std::move(response));
	auto self = shared_from_this();
	
	boost::beast::http::async_write(m_socket, *sp,
			boost::asio::bind_executor(m_strand, [sp,self,this](boost::system::error_code ec, std::size_t bytesTransferred) {
				if (sp->need_eof())
					doClose();
				
				doRead();
			}));
}

template void WebSession::send(boost::beast::http::response<boost::beast::http::empty_body>&& response);
template void WebSession::send(boost::beast::http::response<boost::beast::http::string_body>&& response);


void WebSession::handleRequest()
{
	// Returns a bad request response
    auto const bad_request =
    [&](boost::beast::string_view why)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, m_request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(m_request.keep_alive());
        res.body() = why.to_string();
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&](boost::beast::string_view target)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, m_request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(m_request.keep_alive());
        res.body() = "The resource '" + target.to_string() + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&](boost::beast::string_view what)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, m_request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(m_request.keep_alive());
        res.body() = "An error occurred: '" + what.to_string() + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( m_request.method() != boost::beast::http::verb::get &&
        m_request.method() != boost::beast::http::verb::head &&
		m_request.method() != boost::beast::http::verb::post)
	{
        return send(bad_request("Unknown HTTP-method"));
	}

    // Request path must be absolute and not contain "..".
    if( m_request.target().empty() ||
        m_request.target()[0] != '/' ||
        m_request.target().find("..") != boost::beast::string_view::npos)
	{
        return send(bad_request("Illegal request-target"));
	}
	
	// REST API call handling
	if (m_request.target().starts_with("/api/"))
	{
		try
		{
			return handleRESTCall();
		}
		catch (const WebErrors::not_found& e)
		{
			return send(not_found(e.what()));
		}
		catch (const WebErrors::bad_request& e)
		{
			return send(bad_request(e.what()));
		}
		catch (const std::exception& e)
		{
			return send(server_error(e.what()));
		}
	}
	
	// Static file handling
	if (m_request.method() == boost::beast::http::verb::post)
        return send(bad_request("Unknown HTTP-method"));
	
	std::string path = path_cat(WEBROOT, m_request.target());
    if (m_request.target().back() == '/')
        path.append("index.html");
	
	boost::beast::error_code ec;
    boost::beast::http::file_body::value_type body;
	
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory)
        return send(not_found(m_request.target()));

    // Handle an unknown error
    if(ec)
        return send(server_error(ec.message()));

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(m_request.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, m_request.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(m_request.keep_alive());
        return send(std::move(res));
    }

    // Respond to GET request
    boost::beast::http::response<boost::beast::http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, m_request.version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(m_request.keep_alive());
	
    send(std::move(res));
}

boost::beast::string_view WebSession::mime_type(boost::beast::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == boost::beast::string_view::npos)
            return boost::beast::string_view{};
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

void WebSession::handleRESTCall()
{
	// Remove "/api"
	std::string resource = m_request.target().substr(4).to_string();
	
	// Route the request
	if (!WebRESTHandler::instance().call(resource, m_request, this))
		throw WebErrors::not_found(m_request.target().to_string());
}
