/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PrinterManager.cpp
 * Author: lubos
 * 
 * Created on 28. března 2018, 15:54
 */

#include "PrinterManager.h"
#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "util.h"

PrinterManager::PrinterManager(boost::asio::io_service& io, boost::property_tree::ptree& config)
: m_io(io), m_config(config)
{
	load();
}

PrinterManager::~PrinterManager()
{
}

void PrinterManager::save()
{
	boost::property_tree::ptree& printers = m_config.put_child("printers", boost::property_tree::ptree());
	printers.put("default", m_defaultPrinter);

	for (auto it : m_printers)
	{
		boost::property_tree::ptree& printer = printers.put_child(it.first, boost::property_tree::ptree());
		it.second->save(printer);
	}

	m_config.put("octoprint_api_key", m_octoprintApiKey);

	saveConfig();
}

void PrinterManager::load()
{
	const boost::optional<boost::property_tree::ptree&> printers = m_config.get_child_optional("printers");
	if (printers)
	{
		m_defaultPrinter = printers->get<std::string>("default");

		for (auto it : *printers)
		{
			if (it.second.empty()) // not a subtree
				continue;

			std::shared_ptr<Printer> printer = std::make_shared<Printer>(m_io);
			printer->setUniqueName(it.first.c_str());
			printer->load(it.second);

			m_printers.emplace(it.first, printer);
		}
	}

	m_octoprintApiKey = m_config.get("octoprint_api_key", "");
	if (m_octoprintApiKey.empty())
		regenerateApiKey();
}

void PrinterManager::saveSettings()
{
	std::unique_lock<std::mutex> lock(m_printersMutex);
	save();
}

void PrinterManager::regenerateApiKey()
{
	static boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();

	m_octoprintApiKey = boost::uuids::to_string(u);
}

std::shared_ptr<Printer> PrinterManager::newPrinter()
{
	return std::make_shared<Printer>(m_io);
}

std::shared_ptr<Printer> PrinterManager::printer(const char* name)
{
	std::unique_lock<std::mutex> lock(m_printersMutex);
	std::shared_ptr<Printer> rv;
	
	auto it = m_printers.find(name);
	if (it != m_printers.end())
		rv = it->second;
	
	return rv;
}

void PrinterManager::addPrinter(std::shared_ptr<Printer> printer)
{
	std::unique_lock<std::mutex> lock(m_printersMutex);
	std::string origName = printer->uniqueName();
	int idx = 2;
	
	while (m_printers.find(printer->uniqueName()) != m_printers.end())
	{
		std::string newName = origName + std::to_string(idx++);
		printer->setUniqueName(newName.c_str());
	}

	BOOST_LOG_TRIVIAL(info) << "Adding a new printer, unique name: \""
							<< printer->uniqueName() << "\", device: " << printer->devicePath();
	
	m_printers.insert(std::make_pair(printer->uniqueName(), printer));
	if (m_defaultPrinter.empty())
		m_defaultPrinter = printer->uniqueName();

	save();
	m_printerListChangeSignal();
}

bool PrinterManager::deletePrinter(const char* name)
{
	std::unique_lock<std::mutex> lock(m_printersMutex);
	auto it = m_printers.find(name);
	
	if (it != m_printers.end())
		m_printers.erase(it);
	else
		return false;

	BOOST_LOG_TRIVIAL(info) << "Printer " << name << "has been deleted";

	if (name == m_defaultPrinter)
	{
		if (!m_printers.empty())
			m_defaultPrinter = m_printers.begin()->first;
		else
			m_defaultPrinter.clear();
	}

	save();
	m_printerListChangeSignal();

	return true;
}

std::set<std::string> PrinterManager::printerNames() const
{
	std::set<std::string> rv;
	std::unique_lock<std::mutex> lock(m_printersMutex);
	
	for (auto const& item : m_printers)
		rv.insert(item.first);
	
	return rv;
}

void PrinterManager::setDefaultPrinter(const char* name)
{
	m_defaultPrinter = name;
}

const char* PrinterManager::defaultPrinter()
{
	return m_defaultPrinter.c_str();
}
