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

#include "RestApi.h"
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

	void restPrinters(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		nlohmann::json result = nlohmann::json::object();

		std::set<std::string> names = printerManager->printerNames();
		std::string defaultPrinter = printerManager->defaultPrinter();

		for (const std::string& name : names)
		{
			std::shared_ptr<Printer> printer = printerManager->printer(name.c_str());

			if (!printer)
				continue;

			result[name] = jsonFillPrinter(printer, printer->uniqueName() == defaultPrinter);
		}

		resp.send(result);
	}

	void restDeletePrinter(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::string name = req.pathParam(1);
		if (!printerManager->deletePrinter(name.c_str()))
			throw WebErrors::not_found("Unknown printer");
		else
			resp.send(WebResponse::http_status::no_content);
	}

	void configurePrinterFromJson(nlohmann::json& data, Printer* printer, bool& makeDefault, bool startByDefault)
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

	void restSetupNewPrinter(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::shared_ptr<Printer> printer = printerManager->newPrinter();
		nlohmann::json data = req.jsonRequest();
		bool makeDefault = false;

		if (data["name"].get<std::string>().empty() || data["device_path"].get<std::string>().empty() || data["baud_rate"].get<int>() < 1200)
			throw WebErrors::bad_request("Missing parameters");

		configurePrinterFromJson(data, printer.get(), makeDefault, true);
		printer->setUniqueName(urlSafeString(printer->name(), "printer").c_str());

		printerManager->addPrinter(printer);

		if (makeDefault)
			printerManager->setDefaultPrinter(printer->uniqueName());

		printerManager->saveSettings();

		std::stringstream ss;
		ss << req.baseUrl() << "/api/v1/printers/" << printer->uniqueName();

		resp.set(WebResponse::http_header::location, ss.str());
		resp.send(WebResponse::http_status::created);
	}

	void restSetupPrinter(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		bool newPrinter = false, makeDefault = false;
		std::string name = req.pathParam(1);

		std::shared_ptr<Printer> printer = printerManager->printer(name.c_str());
		nlohmann::json data = req.jsonRequest();

		if (!printer)
		{
			newPrinter = true;
			printer = printerManager->newPrinter();
			printer->setUniqueName(name.c_str());
		}

		configurePrinterFromJson(data, printer.get(), makeDefault, false);

		if (newPrinter)
			printerManager->addPrinter(printer);

		if (makeDefault)
			printerManager->setDefaultPrinter(name.c_str());

		printerManager->saveSettings();

		resp.send(WebResponse::http_status::no_content);
	}

	void restSubmitJob(WebRequest& req, WebResponse& resp, PrinterManager* printerManager, FileManager* fileManager)
	{
		// If there's a paused/running print job, report a conflict
		// else create a new PrintJob based on the file name passed and start it.
		nlohmann::json jreq = req.jsonRequest();

		if (!jreq["file"].is_string())
			throw WebErrors::bad_request("missing 'file' param");

		std::string printerName = req.pathParam(1);
		std::shared_ptr<Printer> printer = printerManager->printer(printerName.c_str());

		if (!printer)
			throw WebErrors::not_found("Printer not found");

		std::shared_ptr<PrintJob> printJob = printer->printJob();
		if (printJob && (printJob->state() == PrintJob::State::Paused || printJob->state() == PrintJob::State::Running))
		{
			resp.send(WebResponse::http_status::conflict);
			return;
		}

		std::string filePath = fileManager->getFilePath(jreq["file"].get<std::string>().c_str());
		if (!boost::filesystem::is_regular_file(filePath))
			throw WebErrors::not_found(".gcode file not found");
		
		printJob = std::make_shared<PrintJob>(printer, filePath.c_str());
		printer->setPrintJob(printJob);

		// Unless state is Stopped, start the job
		if (!jreq["state"].is_string() || jreq["state"].get<std::string>() != "Stopped")
			printJob->start();

		resp.send(WebResponse::http_status::no_content);
	}

	void restModifyJob(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		nlohmann::json jreq = req.jsonRequest();
		std::string printerName = req.pathParam(1);
		std::shared_ptr<Printer> printer = printerManager->printer(printerName.c_str());

		if (!printer)
			throw WebErrors::not_found("Printer not found");

		std::shared_ptr<PrintJob> printJob = printer->printJob();
		if (!printJob)
			throw WebErrors::not_found("Print job not found");
		
		if (jreq["state"].is_string())
		{
			std::string stateValue = jreq["state"].get<std::string>();
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

		resp.send(WebResponse::http_status::no_content);
	}

	void restGetJob(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::string printerName = req.pathParam(1);
		std::shared_ptr<Printer> printer = printerManager->printer(printerName.c_str());

		// TODO
	}

	void restGetPrinterTemperatures(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::string name = req.pathParam(1);

		std::shared_ptr<Printer> printer = printerManager->printer(name.c_str());
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

		resp.send(result, WebResponse::http_status::ok);
	}

	void restSetPrinterTemperatures(WebRequest& req, WebResponse& resp, PrinterManager* printerManager)
	{
		std::string name = req.pathParam(1);

		std::shared_ptr<Printer> printer = printerManager->printer(name.c_str());
		if (!printer)
			throw WebErrors::not_found("Printer not found");

		if (printer->state() != Printer::State::Connected)
			throw WebErrors::bad_request("Printer not connected");

		nlohmann::json changes = req.jsonRequest();
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
					resp.send(WebResponse::http_status::no_content);
			});
		}

		if (commandsToGo == 0)
			resp.send(WebResponse::http_status::no_content);
	}


	void restListFiles(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::vector<FileManager::FileInfo> files = fileManager->listFiles();
		nlohmann::json result = nlohmann::json::array();

		for (const FileManager::FileInfo& file : files)
		{
			nlohmann::json ff = nlohmann::json::object();

			ff["name"] = file.name;
			ff["length"] = file.length;
			ff["mtime"] = file.modifiedTime;

			result.push_back(ff);
		}

		resp.send(result, WebResponse::http_status::ok);
	}

	void restUploadFile(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::string name = req.pathParam(1);
		
		if (req.hasRequestFile())
			fileManager->saveFile(name.c_str(), req.requestFile().c_str());
		else
			fileManager->saveFile(name.c_str(), req.request().body().c_str(), req.request().body().length());

		resp.send(boost::beast::http::status::created);
	}

	void restDownloadFile(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::string name = req.pathParam(1);
		std::string path = fileManager->getFilePath(name.c_str());

		resp.sendFile(path.c_str());
	}


}

void routeRest(std::shared_ptr<WebRouter> router, FileManager& fileManager, PrinterManager& printerManager)
{
	router->post("printers/discover", restPrintersDiscover);
	router->get("printers", restPrinters, &printerManager);
	router->post("printers", restSetupNewPrinter, &printerManager);
	
	router->put("printers/([^/]+)", restSetupPrinter, &printerManager);
	router->get("printers/([^/]+)", restPrinter, &printerManager);
	router->delete_("printers/([^/]+)", restDeletePrinter, &printerManager);

	router->post("printers/([^/]+)/job", restSubmitJob, &printerManager, &fileManager);
	router->put("printers/([^/]+)/job", restModifyJob, &printerManager);
	router->get("printers/([^/]+)/job", restGetJob, &printerManager);

	router->put("printers/([^/]+)/temperatures", restSetPrinterTemperatures, &printerManager);
	router->get("printers/([^/]+)/temperatures", restGetPrinterTemperatures, &printerManager);

	router->post("files/([^/]+)", restUploadFile, &fileManager);
	router->get("files/([^/]+)", restDownloadFile, &fileManager);
	router->get("files", restListFiles, &fileManager);

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
