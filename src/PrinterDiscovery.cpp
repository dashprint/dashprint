#include "PrinterDiscovery.h"
#ifdef WITH_UDEV
#include "UdevPrinterDiscovery.h"
#endif
#include "StaticPrinterDiscovery.h"
#include <exception>
#include <iostream>

void PrinterDiscovery::enumerateAll(std::vector<DiscoveredPrinter>& out)
{
#ifdef WITH_UDEV
	try
	{
		UdevPrinterDiscovery u;
		u.enumerate(out);
		return;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error using udev: " << e.what() << std::endl;
	}
#endif
	
	StaticPrinterDiscovery s;
	s.enumerate(out);
}
