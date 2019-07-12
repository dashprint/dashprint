//
// Created by lubos on 13.4.18.
//

#ifndef DASHPRINT_PRINTJOB_H
#define DASHPRINT_PRINTJOB_H
#include <fstream>
#include <memory>
#include <boost/signals2.hpp>
#include <mutex>
#include <string_view>
#include "Printer.h"

class PrintJob : public std::enable_shared_from_this<PrintJob>
{
public:
	PrintJob(std::shared_ptr<Printer> printer, std::string_view fileName, const char* filePath);

	void start();
	void stop();
	void pause();

	enum class State
	{
		Stopped,
		Paused,
		Running,
		Error,
		Done
	};
	State state() const { return m_state; }
	const char* stateString() const;
	static const char* stateString(State state);
	std::string errorString() const;
	void progress(size_t& pos, size_t& total) const;
	const std::string& name() const { return m_jobName; }

	boost::signals2::signal<void(State, std::string)>& stateChangeSignal() { return m_stateChangeSignal; }
	boost::signals2::signal<void(size_t)>& progressChangeSignal() { return m_progressChangeSignal; }
protected:
	void setError(const char* error);
private:
	void printLine();
	void lineProcessed(const std::vector<std::string>& resp);
	void setState(State state);
	std::string nextLine();
private:
	std::ifstream m_file;
	const std::string m_printerUniqueName;
	std::weak_ptr<Printer> m_printer;
	State m_state = State::Stopped;

	std::string m_errorString;
	mutable std::mutex m_mutex;

	boost::signals2::signal<void(State, std::string)> m_stateChangeSignal;
	boost::signals2::signal<void(size_t)> m_progressChangeSignal;
	size_t m_position = 0, m_size;

	const std::string m_jobName;

	friend class Printer;
};


#endif //DASHPRINT_PRINTJOB_H
