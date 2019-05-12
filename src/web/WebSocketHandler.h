#ifndef _WEBSOCKETHANDLER_H
#define _WEBSOCKETHANDLER_H
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <functional>
#include <string>
#include <string_view>
#include <memory>
#include <queue>

class WebSocketHandler : public std::enable_shared_from_this<WebSocketHandler>
{
protected:
	WebSocketHandler();

	void accept(const boost::beast::http::request<boost::beast::http::string_body>& wsreq, boost::asio::ip::tcp::socket&& socket);
public:
	typedef std::function<void(const std::string&)> handler_t;
	void message(handler_t handler) { m_handler = handler; }
	void send(const std::string_view sv);
	void close();
	boost::asio::strand<boost::asio::executor>* strand() { return m_strand.get(); }
private:
	void accepted();
	void doRead();
	void doWrite();
private:
	handler_t m_handler;

	std::unique_ptr<boost::asio::strand<boost::asio::executor>> m_strand;
	std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> m_stream;

	std::string m_wsIncomingData;
	boost::asio::dynamic_string_buffer<char, std::string::traits_type, std::string::allocator_type> m_wsBuffer;

	boost::asio::const_buffer m_sendBuffer;
	std::string m_sendString;

	std::queue<std::string> m_pendingMessages;
	bool m_writingMessage = false;

	friend class WebRouter;
};

#endif
