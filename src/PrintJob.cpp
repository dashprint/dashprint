//
// Created by lubos on 13.4.18.
//

#include "PrintJob.h"
#include <stdexcept>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

PrintJob::PrintJob(std::shared_ptr<Printer> printer, const char* file)
: m_printer(printer), m_printerUniqueName(printer->uniqueName()), m_jobName(boost::filesystem::path(file).stem().generic_string())
{
	m_file.open(file);

	if (!m_file.is_open())
		throw std::runtime_error("Cannot open GCODE file");

	m_file.seekg(0, std::ios_base::end);
	m_size = m_file.tellg();
}

void PrintJob::start()
{
	if (m_state != State::Paused)
		m_file.seekg(0, std::ios_base::beg);

	setState(State::Running);
	printLine();
}

void PrintJob::stop()
{
	setState(State::Stopped);
	// TODO: Move extruder away?
}

void PrintJob::pause()
{
	setState(State::Paused);
	// TODO: pause sequence (move extruder away)
}

void PrintJob::setError(const char* error)
{
	BOOST_LOG_TRIVIAL(error) << "Print job error on " << m_printerUniqueName << ": "
							 << error;

	std::unique_lock<std::mutex> lock(m_mutex);
	m_errorString = error;
	lock.unlock();

	setState(State::Error);
}

std::string PrintJob::nextLine()
{
	std::string line;

	do
	{
		std::getline(m_file, line);

		std::string::size_type pos = line.find(';');
		if (pos != std::string::npos)
			line.resize(pos);

		boost::algorithm::trim(line);
	}
	while (line.empty() && !m_file.eof() && !m_file.bad());

	m_position = m_file.tellg();
	return line;
}

void PrintJob::printLine()
{
	std::shared_ptr<Printer> printer = m_printer.lock();

	try
	{
		std::string line = nextLine();

		if (!printer)
			throw std::runtime_error("The printer is gone");
		if (printer->state() != Printer::State::Connected)
			throw std::runtime_error("The printer is not connected");

		if (!line.empty())
		{
			// Send another line to the printer
			printer->sendCommand(line.c_str(), std::bind(&PrintJob::lineProcessed, this, std::placeholders::_1));
		}
		else
		{
			// Print job done
			setState(State::Done);
		}
	}
	catch (const std::exception& e)
	{
		setError(e.what());
		return;
	}
}

void PrintJob::lineProcessed(const std::vector<std::string>& resp)
{
	try
	{
		if (resp.empty())
			throw std::runtime_error("Printer gone");

		for (const std::string &line : resp)
		{
			if (boost::starts_with(line, "Error:"))
				throw std::runtime_error(line.substr(6));
			else if (boost::starts_with(line, "Resend:"))
				throw std::runtime_error("Invalid line number requested. Have you reset the printer?");
		}

		if (m_state == State::Running)
			printLine();
	}
	catch (const std::exception &e)
	{
		setError(e.what());
	}
}

void PrintJob::setState(State state)
{
	m_state = state;
	m_stateChangeSignal(state);
}

std::string PrintJob::errorString() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_errorString;
}

const char* PrintJob::stateString() const
{
	switch (m_state)
	{
		case State::Stopped:
			return "Stopped";
		case State::Running:
			return "Running";
		case State::Done:
			return "Done";
		case State::Error:
		default:
			return "Error";
	}
}

void PrintJob::progress(size_t& pos, size_t& total) const
{
	pos = m_position;
	total = m_size;
}
