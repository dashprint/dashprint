//
// Created by lubos on 13.4.18.
//

#ifndef DASHPRINT_PRINTJOB_H
#define DASHPRINT_PRINTJOB_H
#include <fstream>
#include <memory>
#include <boost/signals2.hpp>
#include <mutex>
#include "Printer.h"

class PrintJob : std::enable_shared_from_this<PrintJob>
{
public:
	PrintJob(std::shared_ptr<Printer> printer, const char* file);

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
	std::string errorString() const;
	void progress(size_t& pos, size_t& total) const;

	boost::signals2::signal<void(State)>& stateChangeSignal() { return m_stateChangeSignal; }
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

	boost::signals2::signal<void(State)> m_stateChangeSignal;
	size_t m_position = 0, m_size;

	friend class Printer;
};


#endif //DASHPRINT_PRINTJOB_H
