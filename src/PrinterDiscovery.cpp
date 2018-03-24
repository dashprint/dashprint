/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PrinterDiscovery.cpp
 * Author: lubos
 * 
 * Created on 24. března 2018, 22:57
 */

#include "PrinterDiscovery.h"
#include <stdexcept>
#include <sstream>

PrinterDiscovery::PrinterDiscovery()
{
	m_udev = ::udev_new();
	if (!m_udev)
		throw std::runtime_error("Fatal error accessing udev");
}

PrinterDiscovery::~PrinterDiscovery()
{
	::udev_unref(m_udev);
}

std::string PrinterDiscovery::chooseDeviceLink(const char* devlinks)
{
	std::stringstream ss(devlinks);
	std::string bestlink;
	
	while (ss.good())
	{
		std::string link;
		ss >> link;
		
		if (link.compare(0, 18, "/dev/serial/by-id/") == 0)
			return link;
		
		if (bestlink.empty())
			bestlink = link;
	}
	return bestlink;
}

void PrinterDiscovery::enumerate(std::vector<DiscoveredPrinter>& out)
{
	struct udev_enumerate* ue = nullptr;
	
	out.clear();
	
	try
	{
		ue = ::udev_enumerate_new(m_udev);
		if (!ue)
			throw std::runtime_error("Failed to enumerate devices via udev");
		
		::udev_enumerate_add_match_subsystem(ue, "tty");
		::udev_enumerate_add_match_property(ue, "ID_BUS", "usb");
		::udev_enumerate_scan_devices(ue);
		
		struct udev_list_entry* le;
		
		udev_list_entry_foreach(le, ::udev_enumerate_get_list_entry(ue))
		{
			struct udev_device* device;
			DiscoveredPrinter dp;
			const char* str;
			
			device = ::udev_device_new_from_syspath(m_udev, ::udev_list_entry_get_name(le));
			
			str = ::udev_device_get_property_value(device, "ID_MODEL_ENC");
			if (str != nullptr)
				dp.deviceName = str;
			
			str = ::udev_device_get_property_value(device, "ID_VENDOR_ENC");
			if (str != nullptr)
				dp.deviceVendor = str;
			
			str = ::udev_device_get_property_value(device, "ID_SERIAL");
			if (str != nullptr)
				dp.deviceSerial = str;
			
			dp.devicePath = chooseDeviceLink(::udev_device_get_property_value(device, "DEVLINKS"));
			out.push_back(dp);
			
			::udev_device_unref(device);
		}
	}
	catch (...)
	{
		if (ue)
			::udev_enumerate_unref(ue);
		
		throw;
	}
}
