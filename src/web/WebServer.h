/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebServer.h
 * Author: lubos
 *
 * Created on 27. března 2018, 21:36
 */

#ifndef WEBSERVER_H
#define WEBSERVER_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/regex.hpp>
#include <list>
#include "WebRouter.h"

class WebSession;

class WebServer : public WebRouter
{
public:
	WebServer(boost::asio::io_service& io);
	~WebServer();
	
	void start(int port);

	
private:
	void doAccept();
	void connectionAccepted(boost::system::error_code ec);
private:
	boost::asio::io_service& m_io;

	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_socket;
protected:

	friend class WebSession;
};

#define DECLARE_EXCEPTION_TYPE(name) class name : public std::runtime_error { using std::runtime_error::runtime_error; }

namespace WebErrors
{
	DECLARE_EXCEPTION_TYPE(not_found);
	DECLARE_EXCEPTION_TYPE(bad_request);
	DECLARE_EXCEPTION_TYPE(not_acceptable);
}

#endif /* WEBSERVER_H */

