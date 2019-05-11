#include "WebRouter.h"
#include "WebRequest.h"
#include "WebSession.h"
#include "WebServer.h"
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

	if (auto it = m_subroutes.find(route); it != m_subroutes.end())
		return it->second;
	
	std::shared_ptr<WebRouter> sub = std::make_shared<WebRouter>();
	m_subroutes.insert({ route, sub });
	return sub;
}

void WebRouter::ws(const char* regexp, wshandler_t handler)
{
	handle(method_t::get, regexp, [=](WebRequest& req, WebResponse& res) {
		if (!boost::beast::websocket::is_upgrade(req.request()))
			throw WebErrors::bad_request("WebSocket expected");

		// TODO
	});
}

bool WebRouter::findHandler(const char* url, method_t method, handler_t& handler, boost::smatch& m)
{
	for (auto& [k, v] : m_subroutes)
	{
		if (std::strncmp(url, k.c_str(), k.length()) == 0)
			return v->findHandler(url + k.length(), method, handler, m);
	}

	auto itHandler = std::find_if(m_mappings.begin(), m_mappings.end(),
		[&](const auto& mapping) {
			return method == mapping.method && boost::regex_match(std::string(url), m, mapping.re);
		}
	);

	if (itHandler != m_mappings.end())
	{
		handler = itHandler->handler;
		return true;
	}

	return false;
}
