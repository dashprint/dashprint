#ifndef _WEBROUTER_H
#define _WEBROUTER_H
#include <list>
#include <boost/regex.hpp>
#include <boost/beast.hpp>
#include <map>
#include <memory>

class WebRequest;
class WebResponse;

class WebRouter
{
public:
	typedef std::function<void(WebRequest&,WebResponse&)> handler_t;
	typedef std::function<bool()> wshandler_t;
	typedef boost::beast::http::verb method_t;

	void get(const char* regexp, handler_t handler) { handle(method_t::get, regexp, handler); }
	void post(const char* regexp, handler_t handler) { handle(method_t::post, regexp, handler); }
	void put(const char* regexp, handler_t handler) { handle(method_t::put, regexp, handler); }
	void delete_(const char* regexp, handler_t handler) { handle(method_t::delete_, regexp, handler); }
	void ws(const char* regexp, wshandler_t handler);
	void handle(method_t method, const char* regexp, handler_t handler);
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
