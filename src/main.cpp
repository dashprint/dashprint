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
#include "binfile/api.h"

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
	return path.string();
}

void runApp()
{
	boost::asio::io_service io;
	FileManager fileManager(localStoragePath().c_str());
	PrinterManager printerManager(io, g_config);
	WebServer webServer(io);
	
	webServer.get(".*", [](WebRequest& req, WebResponse& res) {
		if (req.header(boost::beast::http::field::accept_encoding).find("gzip") != boost::string_view::npos)
		{
			// Provide compressed file
			auto fileData = binfile::CompressedAsset(req.path().c_str());
			if (!fileData.has_content())
				throw WebErrors::not_found("File not found");
		}
		else
		{
			// Provide uncompressed file
			auto fileData = binfile::Asset(req.path().c_str());
			if (!fileData.has_content())
				throw WebErrors::not_found("File not found");
		}
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
