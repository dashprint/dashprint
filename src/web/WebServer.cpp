/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebServer.cpp
 * Author: lubos
 * 
 * Created on 27. března 2018, 21:36
 */

#include "WebServer.h"
#include <iostream>
#include <boost/log/trivial.hpp>
#include "WebSession.h"


WebServer::WebServer(boost::asio::io_service& io)
: m_io(io), m_acceptor(io), m_socket(io)
{
}

WebServer::~WebServer()
{
}

void WebServer::start(int port)
{
	BOOST_LOG_TRIVIAL(info) << "Starting web server on port " << port;
	
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v6(), port);
	
	// TODO: add fallback for IPv4 only if open() fails
	m_acceptor.open(endpoint.protocol());
	m_acceptor.set_option(boost::asio::ip::v6_only(false));
	m_acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	
	m_acceptor.bind(endpoint);
	m_acceptor.listen();
	
	doAccept();
}

void WebServer::doAccept()
{
	m_acceptor.async_accept(m_socket, std::bind(&WebServer::connectionAccepted, this, std::placeholders::_1));
}

void WebServer::connectionAccepted(boost::system::error_code ec)
{
	if (ec)
	{
		BOOST_LOG_TRIVIAL(error) << "Error accepting a connection: " << ec;
		return;
	}
	
	// Handle the connection
	std::make_shared<WebSession>(this, std::move(m_socket))->run();
	
	doAccept();
}


