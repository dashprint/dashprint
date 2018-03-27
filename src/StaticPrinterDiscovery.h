/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   StaticPrinterDiscovery.h
 * Author: lubos
 *
 * Created on 24. března 2018, 23:57
 */

#ifndef STATICPRINTERDISCOVERY_H
#define STATICPRINTERDISCOVERY_H
#include "PrinterDiscovery.h"

class StaticPrinterDiscovery : public PrinterDiscovery
{
public:
	void enumerate(std::vector<DiscoveredPrinter>& out) override;
private:

};

#endif /* STATICPRINTERDISCOVERY_H */

