/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebSession.h
 * Author: lubos
 *
 * Created on 27. března 2018, 22:00
 */

#ifndef WEBSESSION_H
#define WEBSESSION_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <stdexcept>
#include <boost/asio/spawn.hpp>

class WebServer;

class WebSession : public std::enable_shared_from_this<WebSession>
{
public:
	WebSession(WebServer* ws, boost::asio::ip::tcp::socket socket);
	~WebSession();
	
	void run();
	WebServer* webServer() { return m_server; }
private:
	void doRead();
	void doClose();
	std::string processLargeBody(uint64_t length);
	bool handleRequest();
	bool handleAuthentication();
	bool handleExpect100Continue();
	// void sendWWWAuthenticate();
	
	static std::string cachePath();
	// static void parseAuthenticationKV(std::string in, std::map<std::string,std::string>& out);

	// Returns true if given HTTP request target doesn't use standard authentication
	// but rather an API key passed in HTTP headers.
	// static bool usesOctoPrintApiKey(boost::beast::string_view url);
protected:
	template<bool isRequest, class Body, class Fields> void send(boost::beast::http::message<isRequest, Body, Fields>&& response);
	const std::string& bodyFile() const { return m_bodyFile; }

	WebServer* m_server;
	boost::asio::ip::tcp::socket m_socket;
	std::unique_ptr<boost::beast::http::request_parser<boost::beast::http::string_body>> m_requestParser;
	boost::beast::http::request<boost::beast::http::string_body> m_request;
	boost::beast::flat_buffer m_buffer;
	boost::asio::strand<boost::asio::executor> m_strand;
	boost::optional<boost::asio::yield_context> m_yield;

	std::string m_bodyFile;
	
	// friend class WebRESTHandler;
	// friend class WebRESTContext;
	friend class WebResponse;
};

extern template void WebSession::send(boost::beast::http::response<boost::beast::http::empty_body>&& response);
extern template void WebSession::send(boost::beast::http::response<boost::beast::http::string_body>&& response);
extern template void WebSession::send(boost::beast::http::response<boost::beast::http::file_body>&& response);
extern template void WebSession::send(boost::beast::http::response<boost::beast::http::span_body<char const>>&& response);

#endif /* WEBSESSION_H */

