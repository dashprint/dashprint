#include <iostream>
#include <cstring>
#include <exception>
#include <boost/asio.hpp>
#include "PrinterDiscovery.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <boost/log/trivial.hpp>
#include "util.h"
#include "web/WebServer.h"
#include "PrinterManager.h"
#include "FileManager.h"
#include "web/WebRequest.h"
#include "web/WebResponse.h"
#include "binfile/api.h"
#include "api/RestApi.h"
#include "api/OctoprintRestApi.h"
#include "api/WebSockets.h"

static void runApp();
static void sanityCheck();

boost::property_tree::ptree g_config;

int main(int argc, const char** argv)
{
	try
	{
		sanityCheck();
		
		loadConfig();
		runApp();
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(fatal) << e.what();
		return 1;
	}
	
	return 0;
}

static std::string localStoragePath()
{
	boost::filesystem::path path(::getenv("HOME"));

	path /= ".local/share/dashprint/files";
	return path.generic_string();
}

void runApp()
{
	boost::asio::io_service io;
	FileManager fileManager(localStoragePath());
	PrinterManager printerManager(io, g_config);
	WebServer webServer(io);

	routeRest(webServer.router("/api/v1/"), fileManager, printerManager);
	routeOctoprintRest(webServer.router("/api"), fileManager, printerManager);
	routeWebSockets(webServer.router("/websocket"), fileManager, printerManager);
	
	// Fallback for static resources
	webServer.get(".*", [](WebRequest& req, WebResponse& res) {
		std::string_view path = req.path();

		if (path == "/")
			path = "index.html";
		
		if (req.header(boost::beast::http::field::accept_encoding).find("gzip") != std::string_view::npos)
		{
			// Provide compressed file
			auto fileData = binfile::compressedAsset(path);
			if (fileData.has_value())
			{
				res.set(boost::beast::http::field::content_encoding, "gzip");
				res.send(*fileData, WebResponse::mimeType(path));
				return;
			}
		}
		
		// Provide uncompressed file
		auto fileData = binfile::asset(path);
		if (!fileData.has_value())
			throw WebErrors::not_found("File not found");
		res.send(*fileData, WebResponse::mimeType(path));
	});

	webServer.start(g_config.get<int>("WebServer.port", 8970));
	
	io.run();
}

void sanityCheck()
{
	const char* home = ::getenv("HOME");
	
	if (!home)
		throw std::runtime_error("HOME is not set!");
	
	if (!boost::filesystem::is_directory(boost::filesystem::path(home)))
		throw std::runtime_error("HOME is not set to an existing directory");
}
