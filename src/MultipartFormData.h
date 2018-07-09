#ifndef _MULTIPART_FORM_DATA_H
#define _MULTIPART_FORM_DATA_H
#include <unordered_map>
#include <string>
#include <functional>
#include <stdint.h>
#include <boost/iostreams/device/mapped_file.hpp>

class MultipartFormData
{
public:
	MultipartFormData(const char* filename);
	~MultipartFormData();

	typedef std::unordered_map<std::string, std::string> Headers_t;
	typedef std::function<bool(const Headers_t& headers, const char* name, const void* data, uint64_t length)> FieldHandler_t;

	void parse(FieldHandler_t handler);

	static void parseKV(const char* input, std::string& value, Headers_t& kv);
private:
	void parseHeaders(size_t& offset, Headers_t& out);
private:
	boost::iostreams::mapped_file m_mapping;
};

#endif
