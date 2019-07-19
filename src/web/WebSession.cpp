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
#include "nlohmann/json.hpp"
#include "WebRequest.h"
#include "WebResponse.h"
#include <cstdlib>
#include <unistd.h>

WebSession::WebSession(WebServer* ws, boost::asio::ip::tcp::socket socket)
: m_server(ws), m_socket(std::move(socket)), m_strand(m_socket.get_executor())
{
}


WebSession::~WebSession()
{
	BOOST_LOG_TRIVIAL(trace) << "WebSession::~WebSession()";
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

bool WebSession::handleExpect100Continue()
{
	if (m_requestParser->get()[boost::beast::http::field::expect] == "100-continue")
	{
		// TODO: Check if we have a handler for this URL and method
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
				boost::beast::http::async_read(m_socket, m_buffer, *m_requestParser, *m_yield);
			}

			m_request = m_requestParser->release();

			if (!handleRequest())
				/*break*/;
			break;
		}
		catch (const std::exception &e)
		{
			doClose();
			break;
		}
	}
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


template<bool isRequest, class Body, class Fields>
void WebSession::send(boost::beast::http::message<isRequest, Body, Fields>&& response)
{
	// Ensure that this and response's lifetime extends until async_write() is complete
	auto sp = std::make_shared<boost::beast::http::message<isRequest, Body, Fields>>(std::move(response));
	auto self = shared_from_this();

	boost::beast::http::async_write(m_socket, *sp, *m_yield);
	run();
}

template void WebSession::send(boost::beast::http::response<boost::beast::http::empty_body>&& response);
template void WebSession::send(boost::beast::http::response<boost::beast::http::string_body>&& response);
template void WebSession::send(boost::beast::http::response<boost::beast::http::file_body>&& response);
template void WebSession::send(boost::beast::http::response<boost::beast::http::span_body<char const>>&& response);

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
		if (m_request.target().empty() ||
			m_request.target()[0] != '/' ||
			m_request.target().find("..") != boost::beast::string_view::npos)
		{
			throw WebErrors::bad_request("Illegal request URL");
		}

		
		std::string target = std::string(m_request.target());
		WebRequest req(m_request, m_bodyFile, shared_from_this());
		WebResponse res(shared_from_this());

		/*WebRouter::handler_t handler;
		if (!m_server->findHandler(target.c_str(), m_request.method(), handler, matches))*/
		m_server->runHandler(req, res);
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
