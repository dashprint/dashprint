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
#include "web/web.h"
#include "nlohmann/json.hpp"
#include "PrinterDiscovery.h"
#include "PrinterManager.h"
#include "util.h"
#include "PrintJob.h"

namespace
{
	void restPrintersDiscover(WebRequest& req, WebResponse& resp)
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
		
		resp.send(result);
	}

	nlohmann::json jsonFillPrinter(std::shared_ptr<Printer> printer, bool isDefault)
	{
		return nlohmann::json {
				{"device_path", printer->devicePath()},
				{"baud_rate",   printer->baudRate()},
				{"stopped",     printer->state() == Printer::State::Stopped},
				//{"api_key",     printer->apiKey()},
				{"name",        printer->name()},
				{"default",     isDefault},
				{"connected",   printer->state() == Printer::State::Connected},
				{"width", printer->printArea().width},
				{"height", printer->printArea().height},
				{"depth", printer->printArea().depth}
		};
	}

	void restPrinter(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::string name = req.pathParam(1);
		std::string defaultPrinter = printerManager->defaultPrinter();
		std::shared_ptr<Printer> printer = printerManager->printer(name.c_str());

		if (!printer)
			throw WebErrors::not_found("Unknown printer");

		resp.send(jsonFillPrinter(printer, printer->name() == defaultPrinter));
	}
}

void routeRest(std::shared_ptr<WebRouter> router, FileManager& fileManager, PrinterManager& printerManager)
{
	auto addArgs = [&](auto origHandler, auto... args) {
		return [=](auto req, auto resp) {
			return origHandler(req, resp, args...);
		};
	};

	router->post("printers/discover", restPrintersDiscover);
	//router->get("printers", passContext(restPrinters, printerManager));
	//router->post("printers", passContext(restSetupNewPrinter));
	
	//router->put("printers/([^/]+)", passContext(restSetupPrinter));
	router->get("printers/([^/]+)", addArgs(restPrinter, &printerManager));
	/*router->delete_("printers/([^/]+)", passContext(restDeletePrinter));

	router->post("printers/([^/]+)/job", passContext(restSubmitJob));
	router->put("printers/([^/]+)/job", passContext(restModifyJob));
	router->get("printers/([^/]+)/job", passContext((restGetJob));

	router->put("printers/([^/]+)/temperatures", passContext(restSetPrinterTemperatures));
	router->get("printers/([^/]+)/temperatures", passContext(restGetPrinterTemperatures));

	router->post("files/([^/]+)", passContext(restUploadFile));
	router->get("files/([^/]+)", passContext(restDownloadFile));
	router->get("files", passContext(restListFiles));*/

/*
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

		HandlerMapping{ std::regex("/v1/files/([^/]+)"), method::post, &WebRESTHandler::restUploadFile },
		HandlerMapping{ std::regex("/v1/files/([^/]+)"), method::get, &WebRESTHandler::restDownloadFile },
		HandlerMapping{ std::regex("/v1/files"), method::get, &WebRESTHandler::restListFiles },
				
		// OctoPrint emu API
		// X-Api-Key
		//
		// Used by Slic3r:
		// api/files/local, http form post with print (true/false), path and file
		// api/version
		HandlerMapping{ std::regex("/version"), method::get, &WebRESTHandler::octoprintGetVersion },
		HandlerMapping{ std::regex("/files/local"), method::post, &WebRESTHandler::octoprintUploadGcode },
	});
	*/
}

#if 0

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
	// If there's a paused/running print job, report a conflict
	// else create a new PrintJob based on the file name passed and start it.
	nlohmann::json req = context.jsonRequest();

	if (!req["file"].is_string())
		throw WebErrors::bad_request("missing 'file' param");

	std::string printerName = context.match()[1];
	std::shared_ptr<Printer> printer = context.printerManager()->printer(printerName.c_str());

	if (!printer)
		throw WebErrors::not_found("Printer not found");

	std::shared_ptr<PrintJob> printJob = printer->printJob();
	if (printJob && (printJob->state() == PrintJob::State::Paused || printJob->state() == PrintJob::State::Running))
	{
		context.send(boost::beast::http::status::conflict);
		return;
	}

	std::string filePath = context.fileManager()->getFilePath(req["file"].get<std::string>().c_str());
	if (!boost::filesystem::is_regular_file(filePath))
		throw WebErrors::not_found(".gcode file not found");
	
	printJob = std::make_shared<PrintJob>(printer, filePath.c_str());
	printer->setPrintJob(printJob);

	// Unless state is Stopped, start the job
	if (!req["state"].is_string() || req["state"].get<std::string>() != "Stopped")
		printJob->start();
}

void WebRESTHandler::restModifyJob(WebRESTContext& context)
{
	nlohmann::json req = context.jsonRequest();
	std::string printerName = context.match()[1];
	std::shared_ptr<Printer> printer = context.printerManager()->printer(printerName.c_str());

	if (!printer)
		throw WebErrors::not_found("Printer not found");

	std::shared_ptr<PrintJob> printJob = printer->printJob();
	if (!printJob)
		throw WebErrors::not_found("Print job not found");
	
	if (req["state"].is_string())
	{
		std::string stateValue = req["state"].get<std::string>();
		if (stateValue == "Stopped")
		{
			switch (printJob->state())
			{
				case PrintJob::State::Running:
				case PrintJob::State::Paused:
					printJob->stop();
					break;
				default:
					;
			}
		}
		else if (stateValue == "Paused")
		{
			if (printJob->state() == PrintJob::State::Running)
				printJob->pause();
		}
		else if (stateValue == "Running")
		{
			if (printJob->state() != PrintJob::State::Running)
				printJob->start();
		}
		else
			throw WebErrors::bad_request("Unrecognized 'state' value");
	}

	context.send(boost::beast::http::status::no_content);
}

void WebRESTHandler::restGetJob(WebRESTContext& context)
{
	std::string printerName = context.match()[1];
	std::shared_ptr<Printer> printer = context.printerManager()->printer(printerName.c_str());
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

	if (printer->state() != Printer::State::Connected)
		throw WebErrors::bad_request("Printer not connected");

	nlohmann::json changes = context.jsonRequest();
	if (!changes.is_object())
		throw WebErrors::bad_request("JSON object expected");

	int commandsToGo = 0;
	for (nlohmann::json::iterator it = changes.begin(); it != changes.end(); it++)
	{
		std::string key = it.key();
		int temp = it.value().get<int>();
		std::string gcode;

		if (key == "T")
			gcode = "M104 S";
		else if (key == "B")
			gcode = "M140 S";
		else
			throw WebErrors::bad_request("Unknown temp target key");

		gcode += std::to_string(temp);

		commandsToGo++;
		printer->sendCommand(gcode.c_str(), [&](const std::vector<std::string>& reply) {
			// TODO: Are there ever any errors reported back by the printers here?

			commandsToGo--;
			if (commandsToGo == 0)
				context.send(WebRESTContext::http_status::no_content);
		});
	}

	if (commandsToGo == 0)
		context.send(WebRESTContext::http_status::no_content);
}
	
void WebRESTHandler::octoprintGetVersion(WebRESTContext& context)
{
	context.send("{\"api\": \"0.1\", \"server\": \"1.0.0-dashprint\"}", "application/json");
}

void WebRESTHandler::octoprintUploadGcode(WebRESTContext& context)
{
	// TODO: Support "path" parameter

	std::unique_ptr<MultipartFormData> mp(context.multipartBody());
	const void* fileData = nullptr;
	size_t fileSize;
	std::string fileName;
	bool print = false;

	if (!mp)
		throw WebErrors::bad_request("multipart data expected");

	mp->parse([&](const MultipartFormData::Headers_t& headers, const char* name, const void* data, uint64_t length) {
		if (std::strcmp(name, "print") == 0)
		{
			if (length == 4 && std::memcmp(data, "true", 4) == 0)
				print = true;
		}
		else if (std::strcmp(name, "file") == 0)
		{
			auto itCDP = headers.find("content-disposition");
			if (itCDP != headers.end())
			{
				std::string value;
				MultipartFormData::Headers_t params;

				MultipartFormData::parseKV(itCDP->second.c_str(), value, params);
				auto it = params.find("filename");

				if (it != params.end())
					fileName = it->second;
			}

			fileData = data;
			fileSize = length;
		}
		return true;
	});

	if (!fileData || fileName.empty())
		throw WebErrors::bad_request("missing fields");

	fileName = context.fileManager()->saveFile(fileName.c_str(), fileData, fileSize);

	nlohmann::json result = nlohmann::json::object();
	result["done"] = true;

	nlohmann::json local = nlohmann::json::object();
	local["name"] = fileName;
	local["origin"] = "local";
	local["path"] = fileName;

	nlohmann::json refs = {
		"download", context.baseUrl() + "/api/v1/files/" + fileName,
		"resource", context.baseUrl() + "/api/files/local/" + fileName
	};

	local["refs"] = refs;
	result["files"] = { "local", local };

	// TODO: Add location header!
	context.send(result, WebRESTContext::http_status::created);
}

void WebRESTHandler::restListFiles(WebRESTContext& context)
{
	std::vector<FileManager::FileInfo> files = context.fileManager()->listFiles();
	nlohmann::json result = nlohmann::json::array();

	for (const FileManager::FileInfo& file : files)
	{
		nlohmann::json ff = nlohmann::json::object();

		ff["name"] = file.name;
		ff["length"] = file.length;
		ff["mtime"] = file.modifiedTime;

		result.push_back(ff);
	}

	context.send(result, WebRESTContext::http_status::ok);
}

void WebRESTHandler::restUploadFile(WebRESTContext& context)
{
	std::string name = context.match()[1];
	
	if (!context.fileBody().empty())
		context.fileManager()->saveFile(name.c_str(), context.fileBody().c_str());
	else
		context.fileManager()->saveFile(name.c_str(), context.request().body().c_str(), context.request().body().length());

	context.send(boost::beast::http::status::created);
}

void WebRESTHandler::restDownloadFile(WebRESTContext& context)
{
	std::string name = context.match()[1];
	std::string path = context.fileManager()->getFilePath(name.c_str());

	context.sendFile(path.c_str());
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

void WebRESTContext::sendFile(const char* path, http_status status)
{
	boost::beast::error_code ec;
	boost::beast::http::basic_file_body<boost::beast::file_posix>::value_type body;

	body.open(path, boost::beast::file_mode::scan, ec);
	if (!body.is_open())
		throw WebErrors::not_found("Cannot open file");

	auto size = body.size();

	boost::beast::http::response<boost::beast::http::file_body> res{
				std::piecewise_construct,
				std::make_tuple(std::move(body)),
				std::make_tuple(boost::beast::http::status::ok, m_request.version())};
	res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
	res.content_length(size);
	res.keep_alive(m_request.keep_alive());

	m_session->send(std::move(res));
}

void WebRESTContext::send(const nlohmann::json& json, http_status status)
{
	send(json.dump(), "application/json", status);
}

#endif
