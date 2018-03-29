#include "PrinterDiscovery.h"
#ifdef WITH_UDEV
#include "UdevPrinterDiscovery.h"
#endif
#include "StaticPrinterDiscovery.h"
#include <exception>
#include <iostream>
#include <boost/log/trivial.hpp>

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
		BOOST_LOG_TRIVIAL(error) << "Error using udev: " << e.what();
	}
#endif
	
	StaticPrinterDiscovery s;
	s.enumerate(out);
}
