#ifndef _WEBROUTER_H
#define _WEBROUTER_H
#include <list>
#include <boost/regex.hpp>
#include <boost/beast.hpp>
#include <map>
#include <memory>
#include <string_view>
#include "WebSocketHandler.h"

class WebRequest;
class WebResponse;

class WebRouter
{
public:
	typedef std::function<void(WebRequest&,WebResponse&)> handler_t;
	typedef std::function<void(WebRequest&,WebResponse&,handler_t)> filter_t;
	typedef std::function<bool(WebSocketHandler&,WebRequest&,WebResponse&)> wshandler_t;
	typedef boost::beast::http::verb method_t;

	template <typename Handler, typename... Args> void handle(method_t method, const char* regexp, Handler handler, Args... args)
	{
		handle(method, regexp, (handler_t) [=](WebRequest& req,WebResponse& res) { handler(req, res, args...); });
	}
	template<> void handle(method_t method, const char* regexp, handler_t handler);

	template <typename Handler, typename... Args> void get(const char* regexp, Handler handler, Args... args)
	{
		handle(method_t::get, regexp, handler, args...);
	}
	template <typename Handler, typename... Args> void post(const char* regexp, Handler handler, Args... args)
	{
		handle(method_t::post, regexp, handler, args...);
	}
	template <typename Handler, typename... Args> void put(const char* regexp, Handler handler, Args... args)
	{
		handle(method_t::put, regexp, handler, args...);
	}
	template <typename Handler, typename... Args> void delete_(const char* regexp, Handler handler, Args... args)
	{
		handle(method_t::delete_, regexp, handler, args...);
	}

	void ws(const char* regexp, wshandler_t handler);
	WebRouter* router(const char* route);

	bool findHandler(std::string_view url, method_t method, handler_t& handler, boost::cmatch& m);
	void runHandler(WebRequest& req, WebResponse& res);

	WebRouter* addFilter(filter_t filter) { m_filters.push_back(filter); return this; }

	template <typename Callable, typename... Args>
	static WebRouter::handler_t inlineFilter(filter_t filter, Callable c, Args... args)
	{
		return [=](WebRequest& req, WebResponse& resp) {
			filter(req, resp, [=](WebRequest& req, WebResponse& resp) {
				c(req, resp, args...);
			});
		};
	}
protected:
	void runHandlerNoFilters(WebRequest& req, WebResponse& res);
private:
	struct url_mapping
	{
		boost::regex re;
		method_t method;
		handler_t handler;
	};
	std::list<url_mapping> m_mappings;

	struct subroute
	{
		std::string prefix;
		std::shared_ptr<WebRouter> router;
	};
	std::list<subroute> m_subroutes;
	std::list<filter_t> m_filters;
};

#endif
