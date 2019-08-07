#ifndef _WEBREQUEST_H
#define _WEBREQUEST_H
#include <boost/beast.hpp>
#include <boost/regex.hpp>
#include "nlohmann/json.hpp"

class MultipartFormData;
class WebSession;

class WebRequest
{
public:
	WebRequest(const boost::beast::http::request<boost::beast::http::string_body>& request,
		const std::string& requestFile,
		std::shared_ptr<WebSession> webSession);

	const boost::beast::http::request<boost::beast::http::string_body>& request() const { return m_request; }

	// Are the contents of the request stored in a file?
	bool hasRequestFile() const { return !m_requestFile.empty(); }
	// Name of that file
	const std::string& requestFile() const { return m_requestFile; }

	nlohmann::json jsonRequest() const;

	// E.g. http://myserver
	std::string baseUrl() const;
	std::string_view host() const { return header(boost::beast::http::field::host); }
	std::string_view path() const { return m_request.target(); }

	std::string_view header(boost::beast::http::field hdr) const { return m_request[hdr]; }

	// Returns the MIME multi-part body or nullptr
	MultipartFormData* multipartBody();

	std::string pathParam(unsigned int sub) const { return m_matches.str(sub); }
	std::string pathParam(const char* sub) const { return m_matches.str(sub); }

	std::map<std::string, std::string>& privateData() { return m_privateData; }
	std::map<std::string, std::string>& queryParams() { return m_queryParams; }

	const std::string& target() const { return m_target; }
	const std::string& queryString() const { return m_queryString; }

	const char* queryParam(const char* name);

	static std::string urlDecode(std::string_view in);
protected:
	boost::asio::ip::tcp::socket&& socket();
	boost::cmatch& matches() { return m_matches; }
private:
	void parseQueryString(const std::string& qs);
	void parseQueryStringKV(std::string_view sv);
private:
	const boost::beast::http::request<boost::beast::http::string_body>& m_request;
	const std::string& m_requestFile;
	boost::cmatch m_matches;
	std::shared_ptr<WebSession> m_webSession;
	std::map<std::string, std::string> m_privateData;
	std::string m_target, m_queryString;
	std::map<std::string, std::string> m_queryParams;

	// for websockets
	friend class WebRouter;
};

#endif
