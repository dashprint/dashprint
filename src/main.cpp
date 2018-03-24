#include <iostream>
#include <cstring>
#include <exception>
#include "PrinterDiscovery.h"

static void discoverPrinters();

int main(int argc, const char** argv)
{
	try
	{
		if (argc > 1)
		{
			if (std::strcmp(argv[1], "--discover") == 0)
			{
				discoverPrinters();
				return 0;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}

void discoverPrinters()
{
	PrinterDiscovery pd;
	std::vector<DiscoveredPrinter> printers;

	pd.enumerate(printers);

	for (const DiscoveredPrinter& p : printers)
	{
		std::cout << "Device path: " << p.devicePath << std::endl;
		std::cout << "Device name: " << p.deviceName << std::endl;
	}
}
