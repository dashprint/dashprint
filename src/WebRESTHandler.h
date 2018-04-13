/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   WebRESTHandler.h
 * Author: lubos
 *
 * Created on 28. března 2018, 0:15
 */

#ifndef WEBRESTHANDLER_H
#define WEBRESTHANDLER_H
#include <boost/beast.hpp>
#include <regex>
#include <vector>

class WebSession;
class WebRESTContext;

class WebRESTHandler
{
private:
	WebRESTHandler();
public:
	static WebRESTHandler& instance();
	
	typedef boost::beast::http::request<boost::beast::http::string_body> reqtype_t;
	bool call(const std::string& url, const reqtype_t& req, WebSession* session);
private:
	void restPrintersDiscover(WebRESTContext& context);
	void restPrinters(WebRESTContext& context);
	void restPrinter(WebRESTContext& context);
	void restSetupPrinter(WebRESTContext& context);
	void restSetupNewPrinter(WebRESTContext& context);
	void restDeletePrinter(WebRESTContext& context);

	void restSubmitJob(WebRESTContext& context);
	void restModifyJob(WebRESTContext& context);
	void restGetJob(WebRESTContext& context);
private:
	typedef void (WebRESTHandler::*handler_t)(WebRESTContext& context);
	
	struct HandlerMapping
	{
		std::regex regex;
		boost::beast::http::verb method;
		handler_t handler;
	};
	std::vector<HandlerMapping> m_restHandlers;
};

#endif /* WEBRESTHANDLER_H */

