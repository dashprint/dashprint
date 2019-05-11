#ifndef _MULTIPART_FORM_DATA_H
#define _MULTIPART_FORM_DATA_H
#include <unordered_map>
#include <string>
#include <functional>
#include <string_view>
#include <stdint.h>
#include <boost/iostreams/device/mapped_file.hpp>

class MultipartFormData
{
public:
	MultipartFormData(const char* filename, const char* boundary = nullptr);
	MultipartFormData(const std::string* contents, const char* boundary = nullptr);
	~MultipartFormData();

	typedef std::unordered_map<std::string, std::string> Headers_t;
	typedef std::function<bool(const Headers_t& headers, const char* name, const void* data, uint64_t length)> FieldHandler_t;

	void parse(FieldHandler_t handler);

	static void parseKV(std::string_view input, std::string& value, Headers_t& kv);
private:
	void parseHeaders(size_t& offset, Headers_t& out);

	const char* data() const;
	size_t length() const;
private:
	boost::iostreams::mapped_file m_mapping;
	const std::string* m_contents;
	std::string m_boundary;
};

#endif
