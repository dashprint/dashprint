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
#include "PrintJob.h"
#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <cstring>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/log/trivial.hpp>

#include <ctype.h>
#include <termios.h>

static const boost::posix_time::seconds RECONNECT_DELAY(5);
static const std::chrono::minutes MAX_TEMPERATURE_HISTORY(30);
static constexpr int DATA_TIMEOUT = 5000;
static constexpr int MAX_LINENO = 10000;

Printer::Printer(boost::asio::io_service &io)
		: m_io(io), m_serial(io), m_reconnectTimer(io), m_timeoutTimer(io), m_temperatureTimer(io)
{
}

Printer::~Printer()
{
	reset();
}

void Printer::load(const boost::property_tree::ptree& tree)
{
	m_devicePath = tree.get<std::string>("device_path");
	m_baudRate = tree.get<int>("baud_rate");
	m_name = tree.get<std::string>("name");
	m_printArea.width = tree.get<int>("width");
	m_printArea.height = tree.get<int>("height");
	m_printArea.depth = tree.get<int>("depth");

	if (!tree.get<bool>("stopped"))
		start();
}

void Printer::save(boost::property_tree::ptree& tree)
{
	tree.put("device_path", m_devicePath);
	tree.put("baud_rate", m_baudRate);
	tree.put("name", m_name);
	tree.put("stopped", m_state == State::Stopped);
	tree.put("width", m_printArea.width);
	tree.put("height", m_printArea.height);
	tree.put("depth", m_printArea.depth);
}

const char* Printer::stateName(State state)
{
	switch (state)
	{
		case State::Stopped:
			return "Stopped";
		case State::Disconnected:
			return "Disconnected";
		case State::Initializing:
			return "Initializing";
		case State::Connected:
			return "Connected";
	}
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

void Printer::setPrintArea(PrintArea area)
{
	if (m_printArea != area)
	{
		m_printArea = area;
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
		BOOST_LOG_TRIVIAL(trace) << "Reconnecting to " << m_uniqueName << " due to device settings change";
		doConnect();
	}
}

void Printer::start()
{
	if (m_state != State::Stopped)
		return;

	BOOST_LOG_TRIVIAL(info) << "Printer " << m_uniqueName << " started";
	setState(State::Disconnected);
	doConnect();
}

void Printer::stop()
{
	BOOST_LOG_TRIVIAL(info) << "Printer " << m_uniqueName << " stopped";
	setState(State::Stopped);
	reset();
}

void Printer::reset()
{
	boost::system::error_code ec;

	m_serial.cancel(ec);
	m_serial.close(ec);
	m_reconnectTimer.cancel(ec);
	m_timeoutTimer.cancel(ec);
	m_executingCommand.clear();
	m_temperatures.clear();

	m_replyLines.clear();
	while (!m_commandQueue.empty())
	{
		// m_replyLines is empty, which will indicate failure
		if (m_commandQueue.front().callback)
			m_commandQueue.front().callback(m_replyLines);
		m_commandQueue.pop();
	}
}

void Printer::getTemperature()
{
	if (m_state != State::Connected)
		return;

	sendCommand("M105", [=](const std::vector<std::string>& response) {
		if (!response.empty())
		{
			parseTemperatures(response[response.size()-1]);
		}

		m_temperatureTimer.expires_from_now(boost::posix_time::seconds(5));
		m_temperatureTimer.async_wait([=](const boost::system::error_code& ec) {
			if (!ec)
				getTemperature();
		});
	});
}

void Printer::parseTemperatures(const std::string& line)
{
	std::map<std::string, std::string> values;
	std::map<std::string, float> changes;

	kvParse(line, values);

	TemperaturePoint point;
	TemperaturePoint* prevPoint = nullptr;

	point.when = std::chrono::system_clock::now();

	std::unique_lock<std::mutex> lock(m_temperaturesMutex);
	if (!m_temperatures.empty())
		prevPoint = &m_temperatures.back();

	lock.unlock();

	for (auto it : values)
	{
		// TODO: Multiple tools support
		// TODO: Temp updates (e.g. during preheat) don't contain all the keys as M105 replies
		if (it.first != "T" && it.first != "B")
			continue;

		Temperature& temp = point.values[it.first];
		Temperature* prevTemp = nullptr;
		ssize_t slash;

		if (prevPoint != nullptr)
		{
			auto it2 = prevPoint->values.find(it.first);
			if (it2 != prevPoint->values.end())
				prevTemp = &it2->second;
		}

		try
		{
			float value;

			slash = it.second.find('/');
			value = std::stof(it.second);

			if (!prevTemp || value != prevTemp->current)
				changes.emplace(it.first + ".current", value);
			temp.current = value;

			if (slash != std::string::npos)
			{
				value = std::stof(it.second.substr(slash + 1));

				if (!prevTemp || value != prevTemp->target)
					changes.emplace(it.first + ".target", value);
				temp.target = value;
			}
		}
		catch (const std::exception& e)
		{
			BOOST_LOG_TRIVIAL(warning) << "Failed to parse temperatures on " << m_uniqueName << ": " << line;
		}
	}

	lock.lock();
	m_temperatures.push_back(point);
	while (point.when - m_temperatures.front().when > MAX_TEMPERATURE_HISTORY)
		m_temperatures.pop_front();
	lock.unlock();

	if (!changes.empty())
	{
		m_temperatureChangeSignal(changes);
	}
}

std::list<Printer::TemperaturePoint> Printer::getTemperatureHistory() const
{
	std::unique_lock<std::mutex> lock(m_temperaturesMutex);
	return m_temperatures;
}

void Printer::setState(State state)
{
	if (state != m_state)
	{
		BOOST_LOG_TRIVIAL(trace) << "State change on printer " << m_uniqueName << ": " << stateName(state);

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
		BOOST_LOG_TRIVIAL(debug) << "Opening serial port " << m_devicePath;

		m_serial.open(m_devicePath.c_str());

		::ioctl(m_serial.native_handle(), TIOCEXCL);
		setNoResetOnReopen();

		m_serial.set_option(boost::asio::serial_port_base::baud_rate(m_baudRate));
		m_serial.set_option(boost::asio::serial_port_base::character_size(8));
		m_serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		m_serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		m_serial.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

		setState(State::Initializing);
		doRead();

		// Initial value
		m_lastIncomingData = std::chrono::steady_clock::now();
		setupTimeoutCheck();

		m_reconnectTimer.expires_from_now(boost::posix_time::seconds(1));
		m_reconnectTimer.async_wait([=](const boost::system::error_code& ec) {
			// This is a workaround that helps get rid of trash in the input buffer,
			// but so far things work OK without it.
			// ::usleep(10000);
			// ::tcflush(m_serial.native_handle(), TCIOFLUSH);

			// Issue a 'sleep for 0ms' to stabilize communications.
			// Based on trial-error, it seems to be necessary to do a useless command first after opening the port
			// (USB TTL hardware tends to be quite shoddy), before issuing commands whose results we actually make use of.
			// sendCommand("G4 P0", nullptr);
			sendCommand("M110 N0", nullptr);
			m_nextLineNo = 1;

			// Get printer information
			sendCommand("M115", [=](const std::vector<std::string>& reply) {
				if (reply.size() >= 2)
				{
					kvParse(reply[reply.size() - 2], m_baseParameters);
					for (auto it = m_baseParameters.begin(); it != m_baseParameters.end(); it++)
						std::cout << it->first << " -> " << it->second << std::endl;
				}

				if (!reply.empty())
				{
					setState(State::Connected);
					showStartupMessage();
					getTemperature();
				}
			});

			doWrite();
		});
	}
	catch (const std::exception &e)
	{
		BOOST_LOG_TRIVIAL(error) << "Failed to open serial port " << m_devicePath;

		// Retry in a short while
		m_reconnectTimer.expires_from_now(RECONNECT_DELAY);
		m_reconnectTimer.async_wait([=](const boost::system::error_code& ec) {
			if (!ec)
				doConnect();
		});
	}
}

void Printer::showStartupMessage()
{
	sendCommand("M117 DashPrint connected", nullptr);
}

void Printer::setNoResetOnReopen()
{
	struct termios tos;
	int fd = m_serial.native_handle();

	if (::tcgetattr(fd, &tos) == 0)
	{
		if (tos.c_cflag & HUPCL)
		{
			tos.c_cflag &= ~HUPCL;
			::tcsetattr(fd, TCSANOW, &tos);
		}
	}
}

void Printer::setupTimeoutCheck()
{
	m_timeoutTimer.expires_from_now(boost::posix_time::seconds(1));
	m_timeoutTimer.async_wait(std::bind(&Printer::timeoutCheck, this, std::placeholders::_1));
}

void Printer::timeoutCheck(const boost::system::error_code& ec)
{
	if (ec)
		return;

	if (!m_commandQueue.empty())
	{
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastIncomingData).count() > DATA_TIMEOUT)
		{
			BOOST_LOG_TRIVIAL(error) << "Comm timeout on printer " << m_uniqueName;

			setState(State::Disconnected);
			reset();
			doConnect();
			return;
		}
	}

	setupTimeoutCheck();
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

	m_lastIncomingData = std::chrono::steady_clock::now();

	BOOST_LOG_TRIVIAL(debug) << "Read on printer " << m_uniqueName << ": " << line;

	m_replyLines.push_back(line);

	// This is the final line
	if (boost::starts_with(line, "ok ") || line == "ok")
	{
		// Handle command re-sending
		int resendLine = -1;
		for (const std::string& line : m_replyLines)
		{
			if (boost::starts_with(line, "Resend:"))
			{
				std::string lineNo = line.substr(7);
				boost::algorithm::trim(lineNo);

				try
				{
					resendLine = std::stoi(lineNo);
				}
				catch (const std::exception& e)
				{
					BOOST_LOG_TRIVIAL(debug) << "Failed to parse resend line: " << e.what();
				}

				break;
			}
		}

		// Indicates we've just executed an M110 command not present in the queue
		if (m_nextLineNo == 0)
		{
			m_nextLineNo = 1;
		}
		// If the resend is issued for a line we don't have, pass the error on to the requester
		else if (resendLine != m_nextLineNo-1)
		{
			if (!m_commandQueue.empty())
			{
				auto cb = m_commandQueue.front().callback;
				if (cb)
					cb(m_replyLines);
				m_commandQueue.pop();
			}
		}

		m_replyLines.clear();
		m_executingCommand.clear();

		doWrite();
	}
	else if (m_executingCommand == "M190" || m_executingCommand == "M109")
	{
		parseTemperatures(line);
	}
	else if (line == "start")
	{
		if (m_state == State::Connected)
		{
			// Fail running print jobs
			std::unique_lock<std::mutex> lock(m_printJobMutex);
			if (m_printJob)
				m_printJob->setError("Printer reset");

			// Reset line counter
			m_nextLineNo = MAX_LINENO;
			showStartupMessage();
			doWrite();
		}
	}

	doRead();
}

static std::string extractCommandCode(const std::string& cmd)
{
	auto pos = cmd.find(' ');
	if (pos != std::string::npos)
		return cmd.substr(0, pos);
	return cmd;
}

void Printer::doWrite()
{
	if (!m_executingCommand.empty() || m_commandQueue.empty())
		return;

	m_replyLines.clear();

	if (m_nextLineNo < MAX_LINENO)
	{
		const PendingCommand &pc = m_commandQueue.front();
		std::stringstream ss;

		m_executingCommand = extractCommandCode(pc.command);
		processCommandEffects(pc.command);

		// If this is not a line-setting command
		const bool useLineNumber = m_executingCommand != "M110";
		if (useLineNumber)
		{
			// Prepend next line number
			ss << 'N' << m_nextLineNo++ << ' ';
		}

		ss << pc.command;

		if (useLineNumber)
		{
			ss << ' ';

			unsigned int cs = checksum(ss.str());
			ss << '*' << cs;
		}

		ss << '\n';

		m_commandBuffer = ss.str();
	}
	else
	{
		// Reset the line counter
		m_commandBuffer = "M110 N0\n";
		m_executingCommand = "M110";
		m_nextLineNo = 0; // Will bump to 1 in readDone()
	}

	BOOST_LOG_TRIVIAL(debug) << "Write on printer " << m_uniqueName << ": " << m_commandBuffer.substr(0, m_commandBuffer.length()-1);

	boost::asio::async_write(m_serial, boost::asio::buffer(m_commandBuffer.c_str(), m_commandBuffer.length()),
		std::bind(&Printer::writeDone, this, std::placeholders::_1));

	// Reset timeout calculation
	m_lastIncomingData = std::chrono::steady_clock::now();
}

unsigned int Printer::checksum(std::string cmd)
{
	unsigned int cs = 0;

	for (int i = 0; i < cmd.length() && cmd[i] != '*'; i++)
		cs ^= unsigned(cmd[i]);

	return cs & 0xff;
}

void Printer::writeDone(const boost::system::error_code& ec)
{
	if (ec)
	{
		ioError(ec);
		return;
	}

	// Now we wait for the reply to arrive.
	// Only then we clear m_executingCommand.
}

void Printer::processCommandEffects(const std::string& line)
{
	if (m_executingCommand == "M104" || m_executingCommand == "M109")
	{
		// Extruder target temp change
		if (line.length() > 6 && line[5] == 'S')
			Printer::processTargetTempSetting("T", line);
	}
	else if (m_executingCommand == "M140" || m_executingCommand == "M190")
	{
		// Heatbed target temp change
		if (line.length() > 6 && line[5] == 'S')
			Printer::processTargetTempSetting("B", line);
	}
}

void Printer::processTargetTempSetting(const char* elem, const std::string& line)
{
	// std::unique_lock<std::mutex> lock(m_temperaturesMutex);

	try
	{
		Temperature temp = getTemperatures()[elem];
		float value = std::stof(line.substr(6));

		if (value != temp.target)
		{
			// temp.target = value; // will get automatically updated soon
			m_temperatureChangeSignal({ { std::string(elem) + ".target", value } });
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Failed to parse target temp for command: " << line;
	}

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
		while (x >= 0 && !isspace(line[x]))
		{
			if (line[x] == ':')
				goto ignore;
			x--;
		}

		keyPositions.push_back(std::make_pair(x+1, pos));

ignore:
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

/*
void Printer::regenerateApiKey()
{
	static boost::uuids::random_generator gen;
	boost::uuids::uuid u = gen();

	m_apiKey = boost::uuids::to_string(u);
}
*/

std::map<std::string, Printer::Temperature> Printer::getTemperatures() const
{
	std::lock_guard<std::mutex> lock(m_temperaturesMutex);
	if (!m_temperatures.empty())
		return m_temperatures.back().values;
	else
		return std::map<std::string, Printer::Temperature>();
}

std::shared_ptr<PrintJob> Printer::printJob() const
{
	std::lock_guard<std::mutex> lock(m_printJobMutex);
	return m_printJob;
}

bool Printer::hasPrintJob() const
{
	std::lock_guard<std::mutex> lock(m_printJobMutex);
	return !!m_printJob;
}

void Printer::setPrintJob(std::shared_ptr<PrintJob> job)
{
	std::unique_lock<std::mutex> lock(m_printJobMutex);

	if (job)
	{
		if (m_printJob && (m_printJob->state() == PrintJob::State::Running || m_printJob->state() == PrintJob::State::Paused))
			throw job_exists("Printer already has a print job");

		m_printJob = job;

		lock.unlock();
		m_hasJobChangeSignal(true);
	}
	else if (m_printJob)
	{
		m_printJob.reset();

		lock.unlock();
		m_hasJobChangeSignal(false);
	}
}
