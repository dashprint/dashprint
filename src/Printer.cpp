/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Printer.cpp
 * Author: lubos
 * 
 * Created on 28. března 2018, 15:50
 */

#include "Printer.h"
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/log/trivial.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <ctype.h>

static const boost::posix_time::seconds RECONNECT_DELAY(5);

Printer::Printer(boost::asio::io_service &io)
		: m_io(io), m_serial(io), m_reconnectTimer(io), m_timeoutTimer(io), m_temperatureTimer(io)
{
	regenerateApiKey();
}

Printer::~Printer()
{
	reset();
}

void Printer::load(const boost::property_tree::ptree& tree)
{
	m_devicePath = tree.get<std::string>("device_path");
	m_baudRate = tree.get<int>("baud_rate");
	m_apiKey = tree.get<std::string>("api_key");
	m_name = tree.get<std::string>("name");

	if (!tree.get<bool>("stopped"))
		start();
}

void Printer::save(boost::property_tree::ptree& tree)
{
	tree.put("device_path", m_devicePath);
	tree.put("baud_rate", m_baudRate);
	tree.put("api_key", m_apiKey);
	tree.put("name", m_name);
	tree.put("stopped", m_state == State::Stopped);
}

void Printer::setUniqueName(const char *name)
{
	m_uniqueName = name;
}

void Printer::setDevicePath(const char *devicePath)
{
	if (m_devicePath != devicePath)
	{
		m_devicePath = devicePath;
		deviceSettingsChanged();
	}
}

void Printer::setBaudRate(int rate)
{
	if (m_baudRate != rate)
	{
		m_baudRate = rate;
		deviceSettingsChanged();
	}
}

void Printer::deviceSettingsChanged()
{
	if (m_state != State::Stopped)
	{
		// Reconnect
		reset();
		doConnect();
	}
}

void Printer::start()
{
	if (m_state != State::Stopped)
		return;

	setState(State::Disconnected);
	doConnect();
}

void Printer::stop()
{
	setState(State::Stopped);
	reset();
}

void Printer::reset()
{
	boost::system::error_code ec;

	m_serial.cancel(ec);
	m_serial.close(ec);
	m_reconnectTimer.cancel(ec);

	m_replyLines.clear();
	while (!m_commandQueue.empty())
	{
		// m_replyLines is empty, which will indicate failure
		m_commandQueue.front().callback(m_replyLines);
		m_commandQueue.pop();
	}
	m_executingCommand = false;
}

void Printer::getTemperature()
{
	if (m_state != State::Connected)
		return;

	sendCommand("M105", [=](const std::vector<std::string>& response) {
		if (!response.empty())
		{
			std::map<std::string, std::string> values;
			kvParse(response[response.size()-1], values);

			for (auto it : values)
			{
				Temperature temp;
				ssize_t slash;

				slash = it.second.find('/');
				temp.current = std::stof(it.second);

				if (slash != std::string::npos)
					temp.target = std::stof(it.second.substr(slash+1));

				m_temperaturesMutex.lock();
				m_temperatures.emplace(it.first, temp);
				m_temperaturesMutex.unlock();
			}
		}

		m_temperatureTimer.expires_from_now(boost::posix_time::seconds(5));
		m_temperatureTimer.async_wait([=](const boost::system::error_code& ec) {
			if (!ec)
				getTemperature();
		});
	});
}

void Printer::setState(State state)
{
	if (state != m_state)
	{
		BOOST_LOG_TRIVIAL(trace) << "State change on printer " << m_uniqueName << ": " << int(state);

		m_state = state;
		m_stateChangeSignal(state);
	}
}

void Printer::sendCommand(const char* cmd, CommandCallback cb)
{
	std::string cmdCopy(cmd);

	m_io.post([=]() {
		m_commandQueue.push({ std::move(cmdCopy), cb });
		doWrite();
	});
}

void Printer::doConnect()
{
	if (m_state == State::Stopped)
		return;

	try
	{
		BOOST_LOG_TRIVIAL(debug) << "Opening serial port " << m_devicePath << std::endl;

		m_serial.open(m_devicePath.c_str());

		::ioctl(m_serial.native_handle(), TIOCEXCL);

		m_serial.set_option(boost::asio::serial_port_base::baud_rate(m_baudRate));
		m_serial.set_option(boost::asio::serial_port_base::character_size(8));
		m_serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		m_serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		m_serial.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

		setState(State::Initializing);
		doRead();

		// Get printer information
		sendCommand("M115", [=](const std::vector<std::string>& reply) {
			if (reply.size() >= 2)
				kvParse(reply[reply.size() - 2], m_baseParameters);

			if (!reply.empty())
			{
				setState(State::Connected);
				getTemperature();
			}
		});

		doWrite();
	}
	catch (const std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open serial port " << m_devicePath << std::endl;

		// Retry in a short while
		m_reconnectTimer.expires_from_now(RECONNECT_DELAY);
		m_reconnectTimer.async_wait([=](const boost::system::error_code& ec) {
			if (!ec)
				doConnect();
		});
	}
}

void Printer::ioError(const boost::system::error_code& ec)
{
	if (ec != boost::system::errc::operation_canceled)
	{
		BOOST_LOG_TRIVIAL(error) << "I/O error on printer " << m_uniqueName << ": " << ec << std::endl;

		setState(State::Disconnected);
		reset();
		doConnect();
	}
}

void Printer::doRead()
{
	boost::asio::async_read_until(m_serial, m_streamBuf, '\n',
		std::bind(&Printer::readDone, this, std::placeholders::_1));
}

void Printer::readDone(const boost::system::error_code& ec)
{
	if (ec)
	{
		ioError(ec);
		return;
	}

	std::istream is(&m_streamBuf);
	std::string line;

	std::getline(is, line);

	BOOST_LOG_TRIVIAL(trace) << "Read on printer " << m_uniqueName << ": " << line;

	m_replyLines.push_back(line);

	if (boost::starts_with(line, "ok ") || line == "ok")
	{
		m_commandQueue.front().callback(m_replyLines);
		m_replyLines.clear();
		m_executingCommand = false;

		doWrite();
	}

	doRead();
}

void Printer::doWrite()
{
	if (m_executingCommand || m_commandQueue.empty())
		return;

	m_executingCommand = true;
	m_replyLines.clear();

	const PendingCommand& pc = m_commandQueue.front();
	const size_t length = pc.command.length() + 1;

	std::strcpy(m_commandBuffer, pc.command.c_str());
	std::strcat(m_commandBuffer, "\n");

	BOOST_LOG_TRIVIAL(trace) << "Write on printer " << m_uniqueName << ": " << pc.command;

	boost::asio::async_write(m_serial, boost::asio::buffer(m_commandBuffer, length),
		std::bind(&Printer::writeDone, this, std::placeholders::_1));
}

void Printer::writeDone(const boost::system::error_code& ec)
{
	if (ec)
	{
		ioError(ec);
		return;
	}

	// Now we wait for the reply to arrive.
	// Only then we set m_executingCommand to false.
}

void Printer::kvParse(const std::string& line, std::map<std::string,std::string>& values)
{
	int pos = 0;
	std::vector<std::pair<int, int>> keyPositions;

	values.clear();
	while ((pos = line.find(':', pos)) != std::string::npos)
	{
		int x = pos - 1;

		// Found the beginning of the key
		while (x > 0 && !isspace(line[x]))
			x--;

		keyPositions.push_back(std::make_pair(x+1, pos));

		pos++;
	}

	for (size_t i = 0; i < keyPositions.size(); i++)
	{
		std::string key, value;
		const std::pair<int, int>& pos = keyPositions[i];

		key = line.substr(pos.first, pos.second - pos.first);

		if (i + 1 < keyPositions.size())
		{
			int off = pos.second + 1;
			value = line.substr(off, keyPositions[i+1].first - off - 1);
		}
		else
			value = line.substr(pos.second + 1);

		values.emplace(std::move(key), std::move(value));
	}
}

void Printer::regenerateApiKey()
{
	static boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();

	m_apiKey = boost::uuids::to_string(u);
}

std::map<std::string, Printer::Temperature> Printer::getTemperatures() const
{
	std::unique_lock<std::mutex> lock(m_temperaturesMutex);
	return m_temperatures;
}
