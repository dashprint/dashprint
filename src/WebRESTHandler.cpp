/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebRESTHandler.cpp
 * Author: lubos
 * 
 * Created on 28. března 2018, 0:15
 */

#include "WebRESTHandler.h"

WebRESTHandler::WebRESTHandler()
{
	using method = boost::beast::http::verb;
	
	m_restHandlers.assign({
		HandlerMapping{ std::regex("/printers/discover"), method::post, &WebRESTHandler::restPrintersDiscover },
		HandlerMapping{ std::regex("/printers"), method::get, &WebRESTHandler::restPrinters },
	});
}

WebRESTHandler& WebRESTHandler::instance()
{
	static WebRESTHandler instance;
	return instance;
}

bool WebRESTHandler::call(const std::string& url, const reqtype_t& req, WebSession* session)
{
	decltype(m_restHandlers)::iterator it = std::find_if(m_restHandlers.begin(), m_restHandlers.end(), [&](const HandlerMapping& handler) {
		std::smatch match;
		return req.method() == handler.method && std::regex_match(url, match, handler.regex);
	});
	
	if (it == m_restHandlers.end())
		return false;
	
	(this->*(it->handler))(req, session);
	return true;
}

void WebRESTHandler::restPrintersDiscover(const reqtype_t& req, WebSession* session)
{
	
}

void WebRESTHandler::restPrinters(const reqtype_t& req, WebSession* session)
{
	
}
