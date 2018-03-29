#include <iostream>
#include <cstring>
#include <exception>
#include <boost/asio.hpp>
#include "PrinterDiscovery.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <cstdlib>
#include <boost/log/trivial.hpp>
#include "WebServer.h"
#include "PrinterManager.h"

static void discoverPrinters();
static void runApp();
static void sanityCheck();
void loadConfig();

boost::property_tree::ptree g_config;

int main(int argc, const char** argv)
{
	try
	{
		sanityCheck();
		
		if (argc > 1)
		{
			if (std::strcmp(argv[1], "--discover") == 0)
			{
				discoverPrinters();
				return 0;
			}
		}
		
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

void discoverPrinters()
{
	std::vector<DiscoveredPrinter> printers;

	PrinterDiscovery::enumerateAll(printers);

	for (const DiscoveredPrinter& p : printers)
	{
		std::cout << "Device path: " << p.devicePath << std::endl;
		std::cout << "Device name: " << p.deviceName << std::endl;
	}
}

void runApp()
{
	boost::asio::io_service io;
	PrinterManager printerManager(io, g_config);
	WebServer webServer(io, printerManager);
	
	webServer.start(g_config.get<int>("WebServer.port", 8970));
	
	io.run();
}

void loadConfig()
{
	boost::filesystem::path configPath(::getenv("HOME"));
	
	configPath /= ".config/pi3printrc";
	
	try
	{
		boost::property_tree::ini_parser::read_ini(configPath.string(), g_config);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Failed to load configuration: " << e.what();
	}
}

void sanityCheck()
{
	const char* home = ::getenv("HOME");
	
	if (!home)
		throw std::runtime_error("HOME is not set!");
	
	if (!boost::filesystem::is_directory(boost::filesystem::path(home)))
		throw std::runtime_error("HOME is not set to an existing directory");
}
