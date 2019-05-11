#ifndef _WEBROUTER_H
#define _WEBROUTER_H
#include <list>
#include <boost/regex.hpp>
#include <boost/beast.hpp>
#include <map>
#include <memory>
#include "WebSocketHandler.h"

class WebRequest;
class WebResponse;

class WebRouter
{
public:
	typedef std::function<void(WebRequest&,WebResponse&)> handler_t;
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
	std::shared_ptr<WebRouter> router(const char* route);

	bool findHandler(const char* url, method_t method, handler_t& handler, boost::smatch& m);
private:
	struct url_mapping
	{
		boost::regex re;
		method_t method;
		handler_t handler;
	};
	std::list<url_mapping> m_mappings;

	std::map<std::string, std::shared_ptr<WebRouter>> m_subroutes;
};

#endif
