#include "WebSocketHandler.h"
#include <boost/log/trivial.hpp>

WebSocketHandler::WebSocketHandler()
: m_wsBuffer(m_wsIncomingData)
{

}

void WebSocketHandler::accept(const boost::beast::http::request<boost::beast::http::string_body>& wsreq, boost::asio::ip::tcp::socket&& socket)
{
	auto self = shared_from_this();
	m_strand.reset(new boost::asio::strand<boost::asio::executor>(socket.get_executor()));
	m_stream.reset(new boost::beast::websocket::stream<boost::asio::ip::tcp::socket>(std::move(socket)));

	m_stream->async_accept(wsreq,
						 boost::asio::bind_executor(*m_strand, [self,this](boost::system::error_code ec) {
							 if (!ec)
								 accepted();
						 }));
}

void WebSocketHandler::accepted()
{
	m_stream->text(true);
	m_stream->read_message_max(512);

	doRead();
	doWrite();
}

void WebSocketHandler::doRead()
{
	m_wsBuffer.consume(m_wsBuffer.size());

	m_stream->async_read(m_wsBuffer,
		boost::asio::bind_executor(*m_strand, [self=shared_from_this(),this](boost::system::error_code ec, std::size_t bytes) {
			if (ec)
				return;

			if (m_handler)
				m_handler(m_wsIncomingData);

			doRead();
	}));
}

void WebSocketHandler::doWrite()
{
	if (m_writingMessage || m_pendingMessages.empty())
		return;

	m_sendString = m_pendingMessages.front();
	m_sendBuffer = boost::asio::const_buffer(m_sendString.c_str(), m_sendString.length());

	m_stream->async_write(m_sendBuffer, [=](boost::system::error_code ec, std::size_t bytes) {
		m_pendingMessages.pop();
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

void WebSocketHandler::send(const std::string_view sv)
{
	boost::asio::post(*m_strand, [str = std::string(sv),this]() {
		m_pendingMessages.emplace(str);
		doWrite();
	});
}

void WebSocketHandler::close()
{
	m_stream->async_close(boost::beast::websocket::close_reason(), [](boost::beast::error_code ec) {});
}
