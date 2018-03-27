/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PrinterDiscovery.h
 * Author: lubos
 *
 * Created on 24. března 2018, 23:55
 */

#ifndef PRINTERDISCOVERY_H
#define PRINTERDISCOVERY_H
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
	virtual ~PrinterDiscovery() {}
	virtual void enumerate(std::vector<DiscoveredPrinter>& out) = 0;
	
	static void enumerateAll(std::vector<DiscoveredPrinter>& out);
};


#endif /* PRINTERDISCOVERY_H */

