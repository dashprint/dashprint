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

class WebSession : public std::enable_shared_from_this<WebSession>
{
public:
	WebSession(boost::asio::ip::tcp::socket socket);
	~WebSession();
	
	void run();
private:
	void doRead();
	void onRead(boost::system::error_code ec, std::size_t bytesTransferred);
	void doClose();
	void handleRequest();
	
	template<bool isRequest, class Body, class Fields> void send(boost::beast::http::message<isRequest, Body, Fields>&& response);
	
	static boost::beast::string_view mime_type(boost::beast::string_view path);
private:
	boost::asio::ip::tcp::socket m_socket;
	boost::beast::http::request<boost::beast::http::string_body> m_request;
	boost::beast::flat_buffer m_buffer;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
};

#endif /* WEBSESSION_H */

