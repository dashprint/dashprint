#include "WebSockets.h"
#include <memory>
#include <map>
#include <string>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>
#include "PrintJob.h"
#include <iostream>

class WSSubscriptionServer : public std::enable_shared_from_this<WSSubscriptionServer>
{
public:
	WSSubscriptionServer(WebSocketHandler& ws, PrinterManager& printerManager, FileManager& fileManager)
	: m_ws(ws), m_printerManager(printerManager), m_fileManager(fileManager)
	{
	}

	void handleMessage(const std::string& data)
	{
		BOOST_LOG_TRIVIAL(debug) << "Incoming WS message: " << data;

		try
		{
			nlohmann::json req = nlohmann::json::parse(data);

			if (!req["subscribe"].is_null())
			{
				for (auto& v : req["subscribe"])
					handleSubscribeRequest(v);
			}
			if (!req["unsubscribe"].is_null())
			{
				for (auto& v : req["unsubscribe"])
					handleUnsubscribeRequest(v);
			}
		}
		catch (const std::exception& e)
		{
			BOOST_LOG_TRIVIAL(error) << "Error handling WS message: " << e.what();
		}
	}
private:
	// Set up a connection that automatically dies with this instance of WSSubscriptionServer
	template<typename SignalType, typename CallableType>
	boost::signals2::connection selfTrackingConnect(boost::signals2::signal<SignalType>& signal, CallableType handler)
	{
		typedef typename boost::signals2::signal<SignalType>::slot_type slot_type;
		return signal.connect(slot_type(boost::asio::bind_executor(*m_ws.strand(), handler))
							.track_foreign(shared_from_this()));
	}

	void handleSubscribeRequest(const nlohmann::json& request)
	{
		if (request.is_string())
		{
			std::string v = request.get<std::string>();
			std::vector<std::string> route;

			boost::split(route, v, boost::is_any_of("."));

			if (route[0] == "PrinterManager")
			{
				m_subscriptions.emplace(v, selfTrackingConnect(m_printerManager.printerListChangeSignal(),
															std::bind(&WSSubscriptionServer::printerManagerEvent, this)));
				return;
			}
			else if (route[0] == "Printer" && route.size() >= 3)
			{
				std::shared_ptr<Printer> printer = m_printerManager.printer(route[1].c_str());

				if (printer)
				{
					if (route[2] == "state")
					{
						m_subscriptions.emplace(v, selfTrackingConnect(printer->stateChangeSignal(),
							std::bind(&WSSubscriptionServer::printerStateEvent, this, route[1], std::placeholders::_1)));
						return;
					}
					else if (route[2] == "temperature")
					{
						m_subscriptions.emplace(v, selfTrackingConnect(printer->temperatureChangeSignal(),
							std::bind(&WSSubscriptionServer::printerTemperatureEvent, this, route[1], std::placeholders::_1)));
						return;
					}
					else if (route[2] == "job")
					{
						// Subscribe to hasJob signal
						m_subscriptions.emplace(v, selfTrackingConnect(printer->hasPrintJobChangeSignal(),
							std::bind(&WSSubscriptionServer::printerHasJobEvent, this, route[1], std::placeholders::_1)));
						
						// If it currently has a job, also subscribe to that PrintJob
						doSubscribeJob(printer);
						return;
					}
					else if (route[2] == "gcode")
					{
						m_subscriptions.emplace(v, selfTrackingConnect(printer->gcodeSignal(),
							std::bind(&WSSubscriptionServer::printerGcodeEvent, this, route[1], std::placeholders::_1)));
						return;
					}
				}
			}

			BOOST_LOG_TRIVIAL(warning) << "Subscription " << v << " not handled";
		}
	}

	void handleUnsubscribeRequest(const nlohmann::json& request)
	{
		if (request.is_string())
		{
			std::multimap<std::string, boost::signals2::connection>::iterator it;
			while ((it = m_subscriptions.find(request.get<std::string>())) != m_subscriptions.end())
			{
				it->second.disconnect();
				m_subscriptions.erase(it);
			}
		}
	}

	void raiseEvent(const nlohmann::json& event)
	{
		m_ws.send(event.dump());
	}

	void printerManagerEvent()
	{
		nlohmann::json event;
		event["event"]["PrinterManager"] = nlohmann::json::object();

		raiseEvent(event);
	}

	void printerStateEvent(std::string printer, Printer::State state)
	{
		nlohmann::json event;

		event["event"]["Printer." + printer + ".state"] = {
				{"connected", state == Printer::State::Connected},
				{"stopped", state == Printer::State::Stopped}
		};

		raiseEvent(event);
	}

	void printerTemperatureEvent(std::string printer, std::map<std::string, float> changes)
	{
		nlohmann::json event;

		event["event"]["Printer." + printer + ".temperature"] = changes;
		raiseEvent(event);
	}

	void printerGcodeEvent(std::string printer, Printer::GCodeEvent gcode)
	{
		nlohmann::json event;

		event["event"]["Printer." + printer + ".gcode"] = {
			{ "commandId", gcode.commandId },
			{ "outgoing", gcode.outgoing },
			{ "data", gcode.data },
			{ "tag", gcode.tag }
		};
		raiseEvent(event);
	}

	std::shared_ptr<PrintJob> doSubscribeJob(std::shared_ptr<Printer> printer)
	{
		std::shared_ptr<PrintJob> printJob = printer->printJob();
		if (printJob)
		{
			std::string v = std::string("Printer.") + printer->uniqueName() + ".job";

			m_subscriptions.emplace(v, selfTrackingConnect(printJob->stateChangeSignal(),
				std::bind(&WSSubscriptionServer::jobStateChangeEvent, this, printer->uniqueName(), printJob.get(), std::placeholders::_1, std::placeholders::_2)));

			m_subscriptions.emplace(v, selfTrackingConnect(printJob->progressChangeSignal(),
				std::bind(&WSSubscriptionServer::jobProgressEvent, this, printer->uniqueName(), printJob.get(), std::placeholders::_1)));
		}
		return printJob;
	}

	void printerHasJobEvent(std::string printerId, bool hasJob)
	{
		nlohmann::json event;
		nlohmann::json eventObject = {
			{ "hasJob", hasJob }
		};

		if (hasJob)
		{
			// Subscribe to that job object automatically
			// and fill in job information into "event".
			std::shared_ptr<Printer> printer = m_printerManager.printer(printerId);
			std::shared_ptr<PrintJob> printJob = doSubscribeJob(printer);

			if (printJob)
			{
				// Fill in job information
				eventObject["state"] = printJob->stateString();

				size_t progress, total;
				printJob->progress(progress, total);

				eventObject["done"] = progress;
				eventObject["total"] = total;
				eventObject["name"] = printJob->name();
			}
		}

		event["event"]["Printer." + printerId + ".job"] = eventObject;
		raiseEvent(event);
	}

	void jobStateChangeEvent(std::string printer, PrintJob* job, PrintJob::State state, std::string errorString)
	{
		nlohmann::json event;
		nlohmann::json eventObject = {
			{ "state", PrintJob::stateString(state) },
			{ "elapsed", job->timeElapsed().count() }
		};

		if (state == PrintJob::State::Error)
			eventObject["error"] = errorString;

		event["event"]["Printer." + printer + ".job"] = eventObject;
		raiseEvent(event);
	}

	void jobProgressEvent(std::string printer, PrintJob* job, size_t progress)
	{
		nlohmann::json event;

		event["event"]["Printer." + printer + ".job"] = {
			{ "done", progress },
			{ "elapsed", job->timeElapsed().count() }
		};

		raiseEvent(event);
	}

private:
	WebSocketHandler& m_ws;
	PrinterManager& m_printerManager;
	FileManager& m_fileManager;
	std::multimap<std::string, boost::signals2::connection> m_subscriptions;
};

void routeWebSockets(WebRouter* router, FileManager& fileManager, PrinterManager& printerManager, AuthManager& authManager)
{
	router->addFilter([=, &authManager](WebRequest& req,WebResponse& resp, WebRouter::handler_t next) {
		const std::string& sv = req.target();
		auto pos = sv.rfind('/');

		std::string token = std::string(sv.substr(pos+1));

		std::cout << "Checking WS token: " << token << std::endl;
		std::string user = authManager.checkToken(token.c_str());

		if (!user.empty())
		{
			req.privateData()["username"] = user;

			next(req, resp);
			return;
		}

		resp.send(boost::beast::http::status::unauthorized);
	});
	router->ws("(.*)", [&](WebSocketHandler& ws,WebRequest& req,WebResponse& resp) {
		std::shared_ptr<WSSubscriptionServer> h = std::make_shared<WSSubscriptionServer>(ws, printerManager, fileManager);

		ws.message([=](const std::string& msg) {
			//std::shared_ptr<WSSubscriptionServer> inst = h;
			h->handleMessage(msg);
		});

		return true;
	});
}
