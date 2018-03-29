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
#include "PrinterManager.h"

class WebSession;

class WebServer {
public:
	WebServer(boost::asio::io_service& io, PrinterManager& printerManager);
	~WebServer();
	
	void start(int port);
	PrinterManager* printerManager() { return &m_printerManager; }
private:
	void doAccept();
	void connectionAccepted(boost::system::error_code ec);
private:
	boost::asio::io_service& m_io;
	PrinterManager& m_printerManager;
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_socket;
};

#endif /* WEBSERVER_H */

