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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>
#include "WebSession.h"

class WebRESTContext
{
public:
	WebRESTContext(const WebRESTHandler::reqtype_t& req, WebSession* session)
	: m_request(req), m_session(session)
	{	
	}
	
	using http_status = boost::beast::http::status;
	
	void send(http_status status);
	void send(const std::string& text, const char* contentType, http_status status = http_status::ok);
	void send(const boost::property_tree::ptree& json, http_status status = http_status::ok);
private:
	const WebRESTHandler::reqtype_t& m_request;
	WebSession* m_session;
};

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
	
	WebRESTContext context(req, session);
	(this->*(it->handler))(context);
	return true;
}

void WebRESTHandler::restPrintersDiscover(WebRESTContext& context)
{
	
}

void WebRESTHandler::restPrinters(WebRESTContext& context)
{
	
}


void WebRESTContext::send(http_status status)
{
	boost::beast::http::response<boost::beast::http::empty_body> res{ status, m_request.version() };
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        
	res.content_length(0);
	res.keep_alive(m_request.keep_alive());
	
	m_session->send(std::move(res));
}

void WebRESTContext::send(const std::string& text, const char* contentType, http_status status)
{
	boost::beast::http::response<boost::beast::http::string_body> res{status, m_request.version()};
	
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(boost::beast::http::field::content_type, contentType);
	
	res.keep_alive(m_request.keep_alive());
	res.body() = text;
	res.prepare_payload();
	
	m_session->send(std::move(res));
}

void WebRESTContext::send(const boost::property_tree::ptree& json, http_status status)
{
	std::stringstream ss;
	boost::property_tree::write_json(ss, json);
	send(ss.str(), "application/json", status);
}
