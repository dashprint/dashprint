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
#include "FileManager.h"

class WebSession;

class WebServer {
public:
	WebServer(boost::asio::io_service& io, PrinterManager& printerManager, FileManager& fileManager);
	~WebServer();
	
	void start(int port);
	PrinterManager* printerManager() { return &m_printerManager; }
	FileManager* fileManager() { return &m_fileManager; }
private:
	void doAccept();
	void connectionAccepted(boost::system::error_code ec);
private:
	boost::asio::io_service& m_io;
	PrinterManager& m_printerManager;
	FileManager& m_fileManager;
	boost::asio::ip::tcp::acceptor m_acceptor;
	boost::asio::ip::tcp::socket m_socket;
};

#endif /* WEBSERVER_H */

