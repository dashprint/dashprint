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
#include <sstream>
#include "WebSession.h"
#include "WebServer.h"
#include "nlohmann/json.hpp"
#include "PrinterDiscovery.h"
#include "PrinterManager.h"

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
	void send(const nlohmann::json& json, http_status status = http_status::ok);
	
	std::smatch& match() { return m_match; }

	void setPrinterManager(PrinterManager* pm) { m_printerManager = pm; }
	PrinterManager* printerManager() { return m_printerManager; }

	nlohmann::json jsonRequest() const
	{
		if (m_request.at(boost::beast::http::field::content_type) != "application/json")
			throw WebErrors::not_acceptable("JSON expected");

		return nlohmann::json::parse(m_request.body());
	}
private:
	const WebRESTHandler::reqtype_t& m_request;
	WebSession* m_session;
	std::smatch m_match;
	PrinterManager* m_printerManager;
};

WebRESTHandler::WebRESTHandler()
{
	using method = boost::beast::http::verb;
	
	m_restHandlers.assign({
		// Our API
		HandlerMapping{ std::regex("/v1/printers/discover"), method::post, &WebRESTHandler::restPrintersDiscover },
		HandlerMapping{ std::regex("/v1/printers"), method::get, &WebRESTHandler::restPrinters },
		HandlerMapping{ std::regex("/v1/printers/(.+)"), method::put, &WebRESTHandler::restSetupPrinter },
		HandlerMapping{ std::regex("/v1/printers/(.+)"), method::get, &WebRESTHandler::restPrinter },
				
		// OctoPrint emu API
	});
}

WebRESTHandler& WebRESTHandler::instance()
{
	static WebRESTHandler instance;
	return instance;
}

bool WebRESTHandler::call(const std::string& url, const reqtype_t& req, WebSession* session)
{
	WebRESTContext context(req, session);
	
	decltype(m_restHandlers)::iterator it = std::find_if(m_restHandlers.begin(), m_restHandlers.end(), [&](const HandlerMapping& handler) {
		return req.method() == handler.method && std::regex_match(url, context.match(), handler.regex);
	});
	
	if (it == m_restHandlers.end())
		return false;

	context.setPrinterManager(session->webServer()->printerManager());

	(this->*(it->handler))(context);
	return true;
}

void WebRESTHandler::restPrintersDiscover(WebRESTContext& context)
{
	std::vector<DiscoveredPrinter> printers;
	nlohmann::json result = {
		{ "devices", nlohmann::json::array() }
	};
	
	PrinterDiscovery::enumerateAll(printers);
	
	for (const DiscoveredPrinter& p : printers)
	{
		result["devices"].push_back({
			{ "path", p.devicePath },
			{ "name", p.deviceName },
			{ "vendor", p.deviceVendor },
			{ "serial", p.deviceSerial }
		});
	}
	
	context.send(result);
}

void WebRESTHandler::restPrinter(WebRESTContext& context)
{
	std::string name = context.match()[1].str();
	std::string defaultPrinter = context.printerManager()->defaultPrinter();
	std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());

	if (!printer)
		throw WebErrors::not_found("Unknown printer");

	context.send(nlohmann::json {
			{"device_path", printer->devicePath()},
			{"baud_rate",   printer->baudRate()},
			{"stopped",     printer->state() == Printer::State::Stopped},
			{"api_key",     printer->apiKey()},
			{"name",        printer->name()},
			{"default",     name == defaultPrinter},
	});
}

void WebRESTHandler::restPrinters(WebRESTContext& context)
{
	nlohmann::json result = {
		{ "printers", {} }
	};

	std::set<std::string> names = context.printerManager()->printerNames();
	std::string defaultPrinter = context.printerManager()->defaultPrinter();

	for (const std::string& name : names)
	{
		std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());

		if (!printer)
			continue;

		result["printers"][name] = {
				{"device_path", printer->devicePath()},
				{"baud_rate",   printer->baudRate()},
				{"stopped",     printer->state() == Printer::State::Stopped},
				{"api_key",     printer->apiKey()},
				{"name",        printer->name()},
				{"default",     name == defaultPrinter},
		};
	}

	context.send(result);
}

void WebRESTHandler::restSetupPrinter(WebRESTContext& context)
{
	bool newPrinter = false, makeDefault = false;
	std::string name = context.match()[1];

	std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());
	nlohmann::json data = context.jsonRequest();

	if (!printer)
	{
		newPrinter = true;
		printer = context.printerManager()->newPrinter();
		printer->setUniqueName(name.c_str());
	}

	if (data["name"])
		printer->setName(data["name"].get<std::string>().c_str());

	if (data["device_path"])
		printer->setDevicePath(data["device_path"].get<std::string>().c_str());

	if (data["baud_rate"])
		printer->setBaudRate(data["baud_rate"].get<int>());

	if (data["default"] && data["default"].get<bool>())
		makeDefault = true;

	if (data["stopped"])
	{
		bool stop = data["stopped"].get<bool>();
		if (stop != (printer->state() == Printer::State::Stopped))
		{
			if (stop)
				printer->stop();
			else
				printer->start();
		}
	}

	if (newPrinter)
		context.printerManager()->addPrinter(printer);

	if (makeDefault)
		context.printerManager()->setDefaultPrinter(name.c_str());

	context.printerManager()->saveSettings();
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

void WebRESTContext::send(const nlohmann::json& json, http_status status)
{
	send(json.dump(), "application/json", status);
}
