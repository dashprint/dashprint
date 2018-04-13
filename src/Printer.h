/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Printer.h
 * Author: lubos
 *
 * Created on 28. března 2018, 15:50
 */

#ifndef PRINTER_H
#define PRINTER_H
#include <string>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <queue>
#include <functional>
#include <boost/signals2.hpp>
#include <mutex>

class PrintJob;

class Printer
{
public:
	Printer(boost::asio::io_service& io);
	Printer(const Printer& orig) = delete;
	virtual ~Printer();

	void load(const boost::property_tree::ptree& tree);
	void save(boost::property_tree::ptree& tree);
	
	Printer& operator=(const Printer& orig) = delete;
	
	const char* uniqueName() const { return m_uniqueName.c_str(); }
	void setUniqueName(const char* name);
	
	const char* devicePath() const { return m_devicePath.c_str(); }
	void setDevicePath(const char* devicePath);
	
	int baudRate() const { return m_baudRate; }
	void setBaudRate(int rate);

	const char* name() const { return m_name.c_str(); }
	void setName(const char* name) { m_name = name; }

	const char* apiKey() const { return m_apiKey.c_str(); }
	void regenerateApiKey();
	
	// Connect to the printer and maintain the connection
	void start();
	
	// Disconnect from the printer and unschedule from the io_service
	void stop();
	
	enum class State
	{
		Stopped,
		Disconnected,
		Initializing,
		Connected,
	};
	static const char* stateName(State state);
	
	State state() const { return m_state; }

	typedef std::function<void(const std::vector<std::string>& reply)> CommandCallback;
	void sendCommand(const char* cmd, CommandCallback cb);

	boost::signals2::signal<void(State)>& stateChangeSignal() { return m_stateChangeSignal; }

	struct Temperature
	{
		float current = 0;
		float target = 0;
	};
	typedef std::map<std::string, Temperature> Temperatures;
	Temperatures getTemperatures() const;

	boost::signals2::signal<void(std::map<std::string, float>)>& temperatureChangeSignal() { return m_temperatureChangeSignal; }

	struct PrintArea
	{
		int width, height, depth;
		bool operator!=(const PrintArea& that) const
		{
			return width != that.width || height != that.height || depth != that.depth;
		}
	};
	const PrintArea& printArea() const { return m_printArea; }
	void setPrintArea(PrintArea area);

	class job_exists : public std::runtime_error { using std::runtime_error::runtime_error; };

	void setPrintJob(std::shared_ptr<PrintJob> job);
	std::shared_ptr<PrintJob> printJob() const;
	bool hasPrintJob() const;
	boost::signals2::signal<void(bool)>& hasPrintJobChangeSignal() { return m_hasJobChangeSignal; }
private:
	void setState(State state);

	void deviceSettingsChanged();
	void doConnect();

	void doRead();
	void readDone(const boost::system::error_code& ec);

	void doWrite();
	void writeDone(const boost::system::error_code& ec);

	void reset();
	void ioError(const boost::system::error_code& ec);

	void setupTimeoutCheck();
	void timeoutCheck(const boost::system::error_code& ec);

	// Parse 'key:some value' pairs
	static void kvParse(const std::string& line, std::map<std::string,std::string>& values);

	void getTemperature();
	void parseTemperatures(const std::string& line);

	void processCommandEffects(const std::string& line);
	void processTargetTempSetting(const char* elem, const std::string& line);

	static unsigned int checksum(std::string cmd);
private:
	std::string m_uniqueName; // As used in REST API URLs
	std::string m_devicePath, m_name;
	int m_baudRate = 115200;
	State m_state = State::Stopped;
	boost::asio::io_service& m_io;
	boost::asio::serial_port m_serial;
	boost::asio::deadline_timer m_reconnectTimer, m_timeoutTimer, m_temperatureTimer;

	struct PendingCommand
	{
		std::string command;
		CommandCallback callback;
	};
	std::vector<std::string> m_replyLines;
	std::queue<PendingCommand> m_commandQueue;

	std::string m_executingCommand;
	std::string m_commandBuffer;
	boost::asio::streambuf m_streamBuf;

	boost::signals2::signal<void(State)> m_stateChangeSignal;
	boost::signals2::signal<void(std::map<std::string, float>)> m_temperatureChangeSignal;

	// M115 result
	std::map<std::string, std::string> m_baseParameters;

	std::string m_apiKey;
	Temperatures m_temperatures;
	mutable std::mutex m_temperaturesMutex;

	PrintArea m_printArea;
	std::chrono::time_point<std::chrono::steady_clock> m_lastIncomingData;
	int m_nextLineNo = 1;

	std::shared_ptr<PrintJob> m_printJob;
	mutable std::mutex m_printJobMutex;
	boost::signals2::signal<void(bool)> m_hasJobChangeSignal;
};

#endif /* PRINTER_H */

