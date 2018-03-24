/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PrinterDiscovery.h
 * Author: lubos
 *
 * Created on 24. března 2018, 22:57
 */

#ifndef PRINTERDISCOVERY_H
#define PRINTERDISCOVERY_H
#include <libudev.h>
#include <string>
#include <vector>

struct DiscoveredPrinter
{
	std::string devicePath;
	std::string deviceName, deviceVendor, deviceSerial;
};

class PrinterDiscovery
{
public:
	PrinterDiscovery();
	~PrinterDiscovery();
	
	void enumerate(std::vector<DiscoveredPrinter>& out);
private:
	static std::string chooseDeviceLink(const char* devlinks);
	static std::string unescape(const char* string);
private:
	struct udev* m_udev;
};

#endif /* PRINTERDISCOVERY_H */

