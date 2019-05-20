#include "WebRouter.h"
#include "WebRequest.h"
#include "WebSession.h"
#include "WebServer.h"
#include "WebResponse.h"
#include <stdexcept>

template<> void WebRouter::handle(method_t method, const char* regexp, handler_t handler)
{
	m_mappings.emplace_back(url_mapping{
		boost::regex(regexp), method, handler
	});
}

std::shared_ptr<WebRouter> WebRouter::router(const char* route)
{
	if (std::strlen(route) == 0 || std::strcmp(route, "/") == 0)
		throw std::logic_error("Invalid subroute");
	
	std::shared_ptr<WebRouter> sub = std::make_shared<WebRouter>();
	m_subroutes.emplace_back(subroute { route, sub });
	return sub;
}

void WebRouter::ws(const char* regexp, wshandler_t handler)
{
	handle(method_t::get, regexp, [=](WebRequest& req, WebResponse& resp) {

		if (!boost::beast::websocket::is_upgrade(req.request()))
			throw WebErrors::bad_request("WebSocket expected");

		std::shared_ptr<WebSocketHandler> h(new WebSocketHandler);
		if (handler(*h, req, resp))
			h->accept(req.request(), req.socket());
	});
}

bool WebRouter::findHandler(std::string_view url, method_t method, handler_t& handler, boost::cmatch& m)
{
	for (const subroute& sr : m_subroutes)
	{
		if (url.compare(0, sr.prefix.length(), sr.prefix) == 0)
			return sr.router->findHandler(url.substr(sr.prefix.length()), method, handler, m);
	}

	auto itHandler = std::find_if(m_mappings.begin(), m_mappings.end(),
		[&](const auto& mapping) {
			if (method == mapping.method)
			{
				if (boost::regex_match(url.begin(), url.end(), m, mapping.re))
					return true;
			}
			return false;
		}
	);

	if (itHandler != m_mappings.end())
	{
		handler = itHandler->handler;
		return true;
	}

	return false;
}
