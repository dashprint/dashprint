#include "WebSockets.h"
#include <memory>
#include <map>
#include <string>
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>

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
				}
			}

			BOOST_LOG_TRIVIAL(warning) << "Subscription " << v << " not handled";
		}
	}

	void handleUnsubscribeRequest(const nlohmann::json& request)
	{
		if (request.is_string())
		{
			auto it = m_subscriptions.find(request.get<std::string>());
			it->second.disconnect();
			m_subscriptions.erase(it);
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

private:
	WebSocketHandler& m_ws;
	PrinterManager& m_printerManager;
	FileManager& m_fileManager;
	std::map<std::string, boost::signals2::connection> m_subscriptions;
};

void routeWebSockets(std::shared_ptr<WebRouter> router, FileManager& fileManager, PrinterManager& printerManager)
{
	router->ws("", [&](WebSocketHandler& ws,WebRequest& req,WebResponse& resp) {
		std::shared_ptr<WSSubscriptionServer> h = std::make_shared<WSSubscriptionServer>(ws, printerManager, fileManager);

		ws.message([=](const std::string& msg) {
			//std::shared_ptr<WSSubscriptionServer> inst = h;
			h->handleMessage(msg);
		});

		return true;
	});
}
