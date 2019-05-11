#ifndef _WEBREQUEST_H
#define _WEBREQUEST_H
#include <boost/beast.hpp>
#include <boost/regex.hpp>
#include "nlohmann/json.hpp"

class MultipartFormData;

class WebRequest
{
public:
	WebRequest(const boost::beast::http::request<boost::beast::http::string_body>& request, const std::string& requestFile, const boost::smatch& matches) : m_request(request), m_requestFile(requestFile), m_matches(matches) {}

	const boost::beast::http::request<boost::beast::http::string_body>& request() const { return m_request; }

	// Are the contents of the request stored in a file?
	bool hasRequestFile() const { return !m_requestFile.empty(); }
	// Name of that file
	const std::string& requestFile() const { return m_requestFile; }

	nlohmann::json jsonRequest() const;

	// E.g. http://myserver
	std::string baseUrl() const;
	std::string host() const { return header(boost::beast::http::field::host); }
	std::string path() const { return std::string(m_request.target()); }

	std::string header(boost::beast::http::field hdr) const { return m_request.at(hdr).to_string(); }

	// Returns the MIME multi-part body or nullptr
	MultipartFormData* multipartBody();

	std::string pathParam(unsigned int sub) const { return m_matches.str(sub); }
	std::string pathParam(const char* sub) const { return m_matches.str(sub); }
private:
	const boost::beast::http::request<boost::beast::http::string_body>& m_request;
	const std::string& m_requestFile;
	const boost::smatch& m_matches;
};

#endif
