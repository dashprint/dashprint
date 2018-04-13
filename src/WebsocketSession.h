//
// Created by lubos on 13.4.18.
//

#ifndef DASHPRINT_WEBSOCKETSESSION_H
#define DASHPRINT_WEBSOCKETSESSION_H
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include "nlohmann/json.hpp"
#include "WebServer.h"
#include <queue>

class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
public:
	WebsocketSession(WebServer* ws, boost::asio::ip::tcp::socket&& socket);

	void accept(const boost::beast::http::request<boost::beast::http::string_body>& wsreq);
private:
	void accepted();
	void doRead();
	void processMessage();
	void handleSubscribeRequest(const nlohmann::json& request);
	void handleUnsubscribeRequest(const nlohmann::json& request);

	void raiseEvent(const nlohmann::json& event);
	void doWrite();

	void printerManagerEvent();
	void printerStateEvent(std::string printer, Printer::State state);
	void printerTemperatureEvent(std::string printer, std::map<std::string, float> changes);

	// Connect to given signal. The connection will autodestroy once the websocket is closed
	// and the handler will be automatically routed to our socket's strand.
	template<typename SignalType, typename CallableType>
	boost::signals2::connection selfTrackingConnect(boost::signals2::signal<SignalType>& signal, CallableType handler);
private:
	WebServer* m_server;
	boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_stream;

	std::string m_wsIncomingData;
	boost::asio::dynamic_string_buffer<char, std::string::traits_type, std::string::allocator_type> m_wsBuffer;
	std::map<std::string, boost::signals2::connection> m_subscriptions;

	boost::asio::const_buffer m_sendBuffer;
	std::string m_sendString;

	// Events that occured but haven't yet made it to the websocket
	std::queue<nlohmann::json> m_pendingEvents;
	bool m_writingMessage = false;
};


#endif //DASHPRINT_WEBSOCKETSESSION_H
