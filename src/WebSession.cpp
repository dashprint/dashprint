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

#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include "WebSession.h"
#include "WebServer.h"
#include "dashprint_config.h"
#include "WebRESTHandler.h"
#include "file_body_posix.ipp"
#include "nlohmann/json.hpp"
#include "WebsocketSession.h"
#include <cstdlib>
#include <unistd.h>

WebSession::WebSession(WebServer* ws, boost::asio::ip::tcp::socket socket)
: m_server(ws), m_socket(std::move(socket)), m_strand(m_socket.get_executor())
{
}


WebSession::~WebSession()
{
}

void WebSession::run()
{
	auto self = shared_from_this();
	boost::asio::spawn(m_strand, [this, self](boost::asio::yield_context yield) {
		m_yield.emplace(yield);
		doRead();
	});
}

class deletefile
{
public:
	~deletefile()
	{
		if (!m_path.empty())
			::unlink(m_path.c_str());
	}
	void deleteLater(const std::string& path)
	{
		m_path = path;
	}
private:
	std::string m_path;
};

std::string WebSession::processLargeBody(uint64_t length)
{
	std::unique_ptr<char[]> path;
	std::string dir = cachePath();

	path.reset(new char[dir.length() + 13]);
	std::strcpy(path.get(), dir.c_str());
	std::strcat(path.get(), "uploadXXXXXX");

	int fd = ::mkstemp(path.get());
	if (fd == -1)
		throw std::runtime_error("Unable to create temporary file");

	uint64_t read = 0;
	while (read < length)
	{
		size_t num = std::min<size_t>(m_buffer.size(), length-read);

		::write(fd, m_buffer.data().data(), num);

		read += num;
		m_buffer.consume(num);

		boost::asio::async_read(m_socket, m_buffer.prepare(10*1024), *m_yield);
	}

	::close(fd);
	return path.get();
}

bool WebSession::usesOctoPrintApiKey(boost::beast::string_view url)
{
	return boost::algorithm::starts_with(url, "/api/")
			&& ! boost::algorithm::starts_with(url, "/api/v1/");
}

bool WebSession::handleExpect100Continue()
{
	if (m_requestParser->get()[boost::beast::http::field::expect] == "100-continue")
	{
		if (!usesOctoPrintApiKey(m_requestParser->get().target()))
		{
			if (!handleAuthentication())
				return false;
		}

		boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::continue_,
																		 m_requestParser->get().version()};
		res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
		res.keep_alive(true);

		boost::beast::http::async_write(m_socket, res, *m_yield);
	}
	return true;
}

void WebSession::doRead()
{
	auto self = shared_from_this();

	while (true)
	{
		deletefile df;

		try
		{
			m_requestParser.reset(new boost::beast::http::request_parser<boost::beast::http::string_body>());
			m_requestParser->body_limit(30*1024*1024);

			boost::beast::http::async_read_header(m_socket, m_buffer, *m_requestParser, *m_yield);

			// Returns false if precondition failed
			if (!handleExpect100Continue())
				continue;

			boost::optional<uint64_t> length;
			m_requestParser->get().content_length(length);

			if (length && length.value() > 1024 * 1024)
			{
				// Save body to a temporary file
				m_bodyFile = processLargeBody(length.value());
				df.deleteLater(m_bodyFile);
			}
			else
			{
				m_bodyFile.clear();
				boost::beast::http::async_read(m_socket, m_buffer, m_requestParser->base(), *m_yield);
			}

			if (!usesOctoPrintApiKey(m_requestParser->get().target()))
			{
				if (!handleAuthentication())
					continue;
			}

			m_request = m_requestParser->release();

			if (!handleRequest())
				break;
		}
		catch (const std::exception &e)
		{
			doClose();
			break;
		}
	}
}

bool WebSession::handleAuthentication()
{
	// TODO: Implement and use Bearer authentication
#if 0
	// Only URLs that require authentication are passed to this method
	boost::beast::string_view authorization = m_requestParser->get()[boost::beast::http::field::authorization].to_string();

	if (authorization.empty() || !boost::algorithm::starts_with(authorization, "Digest "))
	{
		sendWWWAuthenticate();
		return false;
	}

	std::map<std::string,std::string> params;
	parseAuthenticationKV(authorization.substr(7).to_string(), params);

	// TODO: process parameters
#endif

	return true;
}

void WebSession::parseAuthenticationKV(std::string in, std::map<std::string,std::string>& out)
{
	boost::algorithm::trim(in);
	std::vector<std::string> elems;

	boost::split(elems, in, boost::is_any_of(","));

	// Remove quotes, split as key=value
	for (const std::string& kv : elems)
	{
		size_t pos = kv.find('=');
		if (pos == std::string::npos)
			continue;

		std::string key = kv.substr(0, pos);
		std::string value = kv.substr(pos+1);

		if (boost::algorithm::starts_with(value, "\"") && boost::algorithm::ends_with(value, "\""))
			value = value.substr(1, value.length()-2);

		out[key] = value;
	}
}

void WebSession::sendWWWAuthenticate()
{
	// TODO: send WWW-Authenticate
}

std::string WebSession::cachePath()
{
	const char* home = ::getenv("HOME");
	std::string path = std::string(home) + "/.cache/dashprint/";

	::mkdir(path.c_str(), 0777);
	return path;
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


template<bool isRequest, class Body, class Fields>
void WebSession::send(boost::beast::http::message<isRequest, Body, Fields>&& response)
{
	// Ensure that this and response's lifetime extends until async_write() is complete
	auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>>(std::move(response));
	auto self = shared_from_this();

	boost::beast::http::async_write(m_socket, *sp, *m_yield);
}

template void WebSession::send(boost::beast::http::response<boost::beast::http::empty_body>&& response);
template void WebSession::send(boost::beast::http::response<boost::beast::http::string_body>&& response);

bool WebSession::handleRequest()
{
    BOOST_LOG_TRIVIAL(trace) << "HTTP request: " << m_request;

	boost::beast::http::response<boost::beast::http::string_body> response;

	response.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	response.set(boost::beast::http::field::content_type, "text/html");
	response.keep_alive(m_request.keep_alive());
	response.version(m_request.version());

	try
	{
		// Request path must be absolute and not contain "..".
		if( m_request.target().empty() ||
			m_request.target()[0] != '/' ||
			m_request.target().find("..") != boost::beast::string_view::npos)
		{
			throw WebErrors::bad_request("Illegal request URL");
		}

		// REST API call handling
		// "Our" API requests
		if (m_request.target().starts_with("/api/v1/"))
		{
			// TODO: HTTP digest auth
			handleRESTCall();
			return true;
		}
		else if (m_request.target().starts_with("/api/"))
		{
			// TODO: OctoPrint compat request
			// NOTE: Authentication via API key
		}

		// TODO: HTTP digest auth

		if (m_request.target() == "/websocket" && boost::beast::websocket::is_upgrade(m_request))
		{
			std::make_shared<WebsocketSession>(m_server, std::move(m_socket))->accept(m_request);
			// We need to break out of the loop, there will be no more HTTP requests on this socket
			return false;
		}

		// Static file handling
		if (m_request.method() != boost::beast::http::verb::post &&
			m_request.method() != boost::beast::http::verb::get)
		{
			throw WebErrors::bad_request("Unsupported HTTP method");
		}

		std::string path = path_cat(WEBROOT, m_request.target());
		if (m_request.target().back() == '/')
			path.append("index.html");

		boost::beast::error_code ec;
		boost::beast::http::basic_file_body<boost::beast::file_posix>::value_type body;
		bool gzip_used = false;

		if (m_request.at(boost::beast::http::field::accept_encoding).find("gzip") != boost::string_view::npos)
		{
			// Try a .gz file
			body.open((path + ".gz").c_str(), boost::beast::file_mode::scan, ec);
			if (!ec)
				gzip_used = true;
		}

		if (!body.is_open())
			body.open(path.c_str(), boost::beast::file_mode::scan, ec);

		// Handle the case where the file doesn't exist
		if(ec)
			throw WebErrors::not_found(m_request.target().to_string());

		// Cache the size since we need it after the move
		auto const size = body.size();

		if (m_request.method() == boost::beast::http::verb::head)
		{
			response.result(boost::beast::http::status::ok);
			response.content_length(size);
			response.set(boost::beast::http::field::content_type, mime_type(path));

			send(std::move(response));
		}
		else
		{
			// Respond to GET request
			boost::beast::http::response<boost::beast::http::file_body> res{
					std::piecewise_construct,
					std::make_tuple(std::move(body)),
					std::make_tuple(boost::beast::http::status::ok, m_request.version())};
			res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
			res.set(boost::beast::http::field::content_type, mime_type(path));
			res.content_length(size);
			res.keep_alive(m_request.keep_alive());

			if (gzip_used)
				res.set(boost::beast::http::field::content_encoding, "gzip");

			send(std::move(res));
		}
	}
	catch (const WebErrors::not_found& e)
	{
		response.result(boost::beast::http::status::not_found);
		response.body() = e.what();
		response.prepare_payload();

		send(std::move(response));
	}
	catch (const WebErrors::bad_request& e)
	{
		response.result(boost::beast::http::status::bad_request);
		response.body() = e.what();
		response.prepare_payload();

		send(std::move(response));
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Error executing REST request: " << e.what();

		response.result(boost::beast::http::status::internal_server_error);
		response.body() = e.what();
		response.prepare_payload();

		send(std::move(response));
	}

	return true;
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
	if (!WebRESTHandler::instance().call(resource, m_request, m_bodyFile, this))
		throw WebErrors::not_found(m_request.target().to_string());
}
