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
#include "util.h"

class WebRESTContext
{
public:
	WebRESTContext(const WebRESTHandler::reqtype_t& req, WebSession* session)
	: m_request(req), m_session(session)
	{	
	}
	
	using http_status = boost::beast::http::status;
	
	void send(http_status status);
	void send(http_status status, const std::map<std::string,std::string>& headers);
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

	std::string host() const
	{
		return m_request.at(boost::beast::http::field::host).to_string();
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
		HandlerMapping{ std::regex("/v1/printers"), method::post, &WebRESTHandler::restSetupNewPrinter },

		HandlerMapping{ std::regex("/v1/printers/([^/]+)"), method::put, &WebRESTHandler::restSetupPrinter },
		HandlerMapping{ std::regex("/v1/printers/([^/]+)"), method::get, &WebRESTHandler::restPrinter },
		HandlerMapping{ std::regex("/v1/printers/([^/]+)"), method::delete_, &WebRESTHandler::restDeletePrinter },

		HandlerMapping{ std::regex("/v1/printers/([^/]+)/job"), method::post, &WebRESTHandler::restSubmitJob },
		HandlerMapping{ std::regex("/v1/printers/([^/]+)/job"), method::put, &WebRESTHandler::restModifyJob },
		HandlerMapping{ std::regex("/v1/printers/([^/]+)/job"), method::get, &WebRESTHandler::restGetJob },

		HandlerMapping{ std::regex("/v1/printers/([^/]+)/temperatures"), method::put, &WebRESTHandler::restSetPrinterTemperatures },
		HandlerMapping{ std::regex("/v1/printers/([^/]+)/temperatures"), method::get, &WebRESTHandler::restGetPrinterTemperatures },
				
		// OctoPrint emu API
		// X-Api-Key
		//
		// Used by Slic3r:
		// api/files/local, http form post with print (true/false), path and file
		// api/version
		HandlerMapping{ std::regex("/version"), method::get, &WebRESTHandler::octoprintGetVersion },
		HandlerMapping{ std::regex("/files/local"), method::post, &WebRESTHandler::octoprintUploadGcode },
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
	nlohmann::json result = nlohmann::json::array();
	
	PrinterDiscovery::enumerateAll(printers);
	
	for (const DiscoveredPrinter& p : printers)
	{
		result.push_back({
			{ "path", p.devicePath },
			{ "name", p.deviceName },
			{ "vendor", p.deviceVendor },
			{ "serial", p.deviceSerial }
		});
	}
	
	context.send(result);
}

static nlohmann::json jsonFillPrinter(std::shared_ptr<Printer> printer, bool isDefault)
{
	return nlohmann::json {
			{"device_path", printer->devicePath()},
			{"baud_rate",   printer->baudRate()},
			{"stopped",     printer->state() == Printer::State::Stopped},
			{"api_key",     printer->apiKey()},
			{"name",        printer->name()},
			{"default",     isDefault},
			{"connected",   printer->state() == Printer::State::Connected},
			{"width", printer->printArea().width},
			{"height", printer->printArea().height},
			{"depth", printer->printArea().depth}
	};
}

void WebRESTHandler::restPrinter(WebRESTContext& context)
{
	std::string name = context.match()[1].str();
	std::string defaultPrinter = context.printerManager()->defaultPrinter();
	std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());

	if (!printer)
		throw WebErrors::not_found("Unknown printer");

	context.send(jsonFillPrinter(printer, printer->name() == defaultPrinter));
}

void WebRESTHandler::restDeletePrinter(WebRESTContext& context)
{
	std::string name = context.match()[1].str();
	if (!context.printerManager()->deletePrinter(name.c_str()))
		throw WebErrors::not_found("Unknown printer");
	else
		context.send(WebRESTContext::http_status::no_content);
}

void WebRESTHandler::restPrinters(WebRESTContext& context)
{
	nlohmann::json result = nlohmann::json::object();

	std::set<std::string> names = context.printerManager()->printerNames();
	std::string defaultPrinter = context.printerManager()->defaultPrinter();

	for (const std::string& name : names)
	{
		std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());

		if (!printer)
			continue;

		result[name] = jsonFillPrinter(printer, printer->uniqueName() == defaultPrinter);
	}

	context.send(result);
}

static void configurePrinterFromJson(nlohmann::json& data, Printer* printer, bool& makeDefault, bool startByDefault)
{
	if (data["name"].is_string())
		printer->setName(data["name"].get<std::string>().c_str());

	if (data["device_path"].is_string())
		printer->setDevicePath(data["device_path"].get<std::string>().c_str());

	if (data["baud_rate"].is_number())
		printer->setBaudRate(data["baud_rate"].get<int>());

	if (data["default"].is_boolean() && data["default"].get<bool>())
		makeDefault = true;

	Printer::PrintArea area = printer->printArea();
	if (data["width"].is_number())
		area.width = data["width"].get<int>();
	if (data["height"].is_number())
		area.height = data["height"].get<int>();
	if (data["depth"].is_number())
		area.depth = data["depth"].get<int>();

	printer->setPrintArea(area);

	if (data["stopped"].is_boolean())
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
	else if (startByDefault)
		printer->start();
}

void WebRESTHandler::restSetupNewPrinter(WebRESTContext& context)
{
	std::shared_ptr<Printer> printer = context.printerManager()->newPrinter();
	nlohmann::json data = context.jsonRequest();
	bool makeDefault = false;

	if (data["name"].get<std::string>().empty() || data["device_path"].get<std::string>().empty() || data["baud_rate"].get<int>() < 1200)
		throw WebErrors::bad_request("Missing parameters");

	configurePrinterFromJson(data, printer.get(), makeDefault, true);
	printer->setUniqueName(urlSafeString(printer->name(), "printer").c_str());

	context.printerManager()->addPrinter(printer);

	if (makeDefault)
		context.printerManager()->setDefaultPrinter(printer->uniqueName());

	context.printerManager()->saveSettings();

	std::stringstream ss;
	ss << "http://" << context.host() << "/api/v1/printers/" << printer->uniqueName();

	context.send(WebRESTContext::http_status::created, {std::make_pair("Location", ss.str())});
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

	configurePrinterFromJson(data, printer.get(), makeDefault, false);

	if (newPrinter)
		context.printerManager()->addPrinter(printer);

	if (makeDefault)
		context.printerManager()->setDefaultPrinter(name.c_str());

	context.printerManager()->saveSettings();

	context.send(WebRESTContext::http_status::no_content);
}

void WebRESTHandler::restSubmitJob(WebRESTContext& context)
{
	// TODO: Support both direct upload and referring to an uploaded file
}

void WebRESTHandler::restModifyJob(WebRESTContext& context)
{

}

void WebRESTHandler::restGetJob(WebRESTContext& context)
{

}

void WebRESTHandler::restGetPrinterTemperatures(WebRESTContext& context)
{
	std::string name = context.match()[1];

	std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());
	if (!printer)
		throw WebErrors::not_found("Printer not found");

	nlohmann::json result = nlohmann::json::array();
	std::list<Printer::TemperaturePoint> temps = printer->getTemperatureHistory();

	for (auto it = temps.begin(); it != temps.end(); it++)
	{
		nlohmann::json values = nlohmann::json::object();

		for (auto it2 = it->values.begin(); it2 != it->values.end(); it2++)
		{
			values[it2->first] = {
					{"current", it2->second.current},
					{"target",  it2->second.target}
			};
		}

		std::time_t tt = std::chrono::system_clock::to_time_t(it->when);
		char when[sizeof "2011-10-08T07:07:09.000Z"];
		strftime(when, sizeof when, "%FT%T.000Z", gmtime(&tt));

		nlohmann::json pt = nlohmann::json::object();
		pt["when"] = when;
		pt["values"] = values;
		result.push_back(pt);
	}

	context.send(result, WebRESTContext::http_status::ok);
}

void WebRESTHandler::restSetPrinterTemperatures(WebRESTContext& context)
{
	std::string name = context.match()[1];

	std::shared_ptr<Printer> printer = context.printerManager()->printer(name.c_str());
	if (!printer)
		throw WebErrors::not_found("Printer not found");
	}
	
void WebRESTHandler::octoprintGetVersion(WebRESTContext& context)
{
	context.send("1.0-dashprint", "text/plain");
}

void WebRESTHandler::octoprintUploadGcode(WebRESTContext& context)
{
	// TODO: Parse WWW form upload
	// Fields:
	// print: true/false
	// path: destination path
	// file: file data
}

void WebRESTContext::send(http_status status, const std::map<std::string,std::string>& headers)
{
	boost::beast::http::response<boost::beast::http::empty_body> res{ status, m_request.version() };
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);

	for (auto it = headers.begin(); it != headers.end(); it++)
		res.set(it->first, it->second);

	res.content_length(0);
	res.keep_alive(m_request.keep_alive());

	m_session->send(std::move(res));
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
