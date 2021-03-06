/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PrinterManager.h
 * Author: lubos
 *
 * Created on 28. března 2018, 15:54
 */

#ifndef PRINTERMANAGER_H
#define PRINTERMANAGER_H
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "Printer.h"

class PrinterManager
{
public:
	PrinterManager(boost::asio::io_service& io, boost::property_tree::ptree& config);
	~PrinterManager();

	PrinterManager& operator=(const PrinterManager& that) = delete;

	std::shared_ptr<Printer> newPrinter();
	std::shared_ptr<Printer> printer(std::string_view name);
	void addPrinter(std::shared_ptr<Printer> printer);
	bool deletePrinter(std::string_view name);
	std::set<std::string> printerNames() const;

	void setDefaultPrinter(std::string_view name);
	const char* defaultPrinter();

	void saveSettings();
	boost::signals2::signal<void()>& printerListChangeSignal() { return m_printerListChangeSignal; }
	void regenerateApiKey();
private:
	void save();
	void load();
private:
	boost::asio::io_service& m_io;
	boost::property_tree::ptree& m_config;
	std::map<std::string, std::shared_ptr<Printer>, std::less<>> m_printers;
	mutable std::mutex m_printersMutex;
	std::string m_defaultPrinter;

	boost::signals2::signal<void()> m_printerListChangeSignal;
	std::string m_octoprintApiKey;
};

#endif /* PRINTERMANAGER_H */

