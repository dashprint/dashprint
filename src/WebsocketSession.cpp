//
// Created by lubos on 13.4.18.
//

#include "WebsocketSession.h"
#include <boost/log/trivial.hpp>
#include <boost/algorithm/string.hpp>

WebsocketSession::WebsocketSession(WebServer* ws, boost::asio::ip::tcp::socket&& socket)
: m_server(ws), m_strand(socket.get_executor()), m_stream(std::move(socket)), m_wsBuffer(m_wsIncomingData)
{

}

void WebsocketSession::accept(const boost::beast::http::request<boost::beast::http::string_body>& wsreq)
{
	auto self = shared_from_this();

	m_stream.async_accept(wsreq,
						 boost::asio::bind_executor(m_strand, [self,this](boost::system::error_code ec) {
							 if (!ec)
								 accepted();
						 }));
}

void WebsocketSession::accepted()
{
	m_stream.text(true);
	m_stream.read_message_max(512);

	doRead();
}

void WebsocketSession::doRead()
{
	auto self = shared_from_this();

	m_wsIncomingData.clear();

	m_stream.async_read(m_wsBuffer,
					   boost::asio::bind_executor(m_strand, [self,this](boost::system::error_code ec, std::size_t bytes) {
						   if (ec)
							   return;

						   processMessage();

						   doRead();
					   }));
}

void WebsocketSession::processMessage()
{
	try
	{
		nlohmann::json req = nlohmann::json::parse(m_wsIncomingData);
		BOOST_LOG_TRIVIAL(debug) << "Incoming WS message: " << req;

		if (req["subscribe"].is_array())
		{
			for (auto v : req["subscribe"])
				handleSubscribeRequest(v);
		}
		if (req["unsubscribe"].is_array())
		{
			for (auto v : req["unsubscribe"])
				handleUnsubscribeRequest(v);
		}
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Error handling WS message: " << e.what();
	}
}

template<typename SignalType, typename CallableType>
boost::signals2::connection WebsocketSession::selfTrackingConnect(boost::signals2::signal<SignalType>& signal, CallableType handler)
{
	typedef typename boost::signals2::signal<SignalType>::slot_type slot_type;
	return signal.connect(slot_type(boost::asio::bind_executor(m_strand, handler))
						   .track_foreign(shared_from_this()));
}

void WebsocketSession::handleSubscribeRequest(const nlohmann::json& request)
{
	if (request.is_string())
	{
		std::string v = request.get<std::string>();
		std::vector<std::string> route;

		boost::split(route, v, boost::is_any_of("."));

		if (route[0] == "PrinterManager")
		{
			m_subscriptions.emplace(v, selfTrackingConnect(m_server->printerManager()->printerListChangeSignal(),
														   std::bind(&WebsocketSession::printerManagerEvent, this)));
		}
		else if (route[0] == "Printer" && route.size() >= 3)
		{
			std::shared_ptr<Printer> printer = m_server->printerManager()->printer(route[1].c_str());

			if (printer)
			{
				if (route[2] == "state")
				{
					m_subscriptions.emplace(v, selfTrackingConnect(printer->stateChangeSignal(),
						std::bind(&WebsocketSession::printerStateEvent, this, route[2], std::placeholders::_1)));
				}
				else if (route[2] == "temperature")
				{
					m_subscriptions.emplace(v, selfTrackingConnect(printer->temperatureChangeSignal(),
						std::bind(&WebsocketSession::printerTemperatureEvent, this, route[2], std::placeholders::_1)));
				}
			}
		}
	}
}

void WebsocketSession::handleUnsubscribeRequest(const nlohmann::json& request)
{
	if (request.is_string())
	{
		auto it = m_subscriptions.find(request.get<std::string>());
		it->second.disconnect();
		m_subscriptions.erase(it);
	}
}

void WebsocketSession::raiseEvent(const nlohmann::json& event)
{
	m_pendingEvents.emplace(std::move(event));
	doWrite();
}

void WebsocketSession::doWrite()
{
	if (m_writingMessage || m_pendingEvents.empty())
		return;

	m_sendString = m_pendingEvents.front().dump();
	m_sendBuffer = boost::asio::const_buffer(m_sendString.c_str(), m_sendString.length());

	m_stream.async_write(m_sendBuffer, [=](boost::system::error_code ec, std::size_t bytes) {
		m_pendingEvents.pop();
		m_writingMessage = false;

		if (ec)
		{
			BOOST_LOG_TRIVIAL(error) << "Failed to send a websocket message: " << ec;
			return;
		}

		doWrite();
	});
	m_writingMessage = true;
}

void WebsocketSession::printerManagerEvent()
{
	nlohmann::json event = {
			{"event", "PrinterManager"}
	};

	raiseEvent(std::move(event));
}

void WebsocketSession::printerStateEvent(std::string printer, Printer::State state)
{
	nlohmann::json event;

	event["event"]["Printer"][printer]["state"] = {
			{"connected", state == Printer::State::Connected},
			{"stopped", state == Printer::State::Stopped}
	};

	raiseEvent(std::move(event));
}

void WebsocketSession::printerTemperatureEvent(std::string printer, std::map<std::string, float> changes)
{
	nlohmann::json event;

	event["event"]["Printer"][printer]["temperature"] = changes;
	raiseEvent(std::move(event));
}
