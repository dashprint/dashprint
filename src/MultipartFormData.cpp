#include "MultipartFormData.h"
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <regex>
#include <iostream>
#include <ctype.h>

MultipartFormData::MultipartFormData(const char* filename)
{
	m_mapping.open(filename, std::ios_base::in);
	if (!m_mapping.is_open())
	{
		std::stringstream ss;
		ss << "Cannot open " << filename;
		throw std::runtime_error(ss.str());
	}
}

MultipartFormData::~MultipartFormData()
{

}

void MultipartFormData::parse(FieldHandler_t handler)
{
	size_t pos = 0;
	std::string boundary;

	while (pos < m_mapping.size())
	{
		Headers_t headers;
		bool contentSection = true;

		parseHeaders(pos, headers);

		// First section
		if (boundary.empty())
		{
			auto it = headers.find("content-type");
			if (it == headers.end())
			{
				std::cerr << "Content-Type expected\n";
				return;
			}
			
			std::string ctValue;
			Headers_t ctParams;
			parseKV(it->second.c_str(), ctValue, ctParams);

			if (ctValue != "multipart/form-data")
			{
				std::cerr << "Unexpected multiform Content-Type: " << ctValue;
				return;
			}

			auto itBoundary = ctParams.find("boundary");
			if (itBoundary == ctParams.end())
			{
				std::cerr << "Cannot determine multipart/form-data boundary!\n";
				return;
			}

			boundary = itBoundary->second;
			boundary.insert(0, "--");

			// Do not invoke callback for this section
			contentSection = false;
		}

		const char* end = static_cast<const char*>(::memmem(m_mapping.const_data() + pos, m_mapping.size() - pos, boundary.c_str(), boundary.length()));
		if (!end || end-m_mapping.const_data() >= m_mapping.size()-2)
		{
			std::cerr << "Premature multipart body end\n";
			break;
		}

		if (contentSection)
		{
			std::string name;
			auto it = headers.find("content-disposition");

			if (it != headers.end())
			{
				Headers_t cdpParams;
				std::string value;

				parseKV(it->second.c_str(), value, cdpParams);

				auto itName = cdpParams.find("name");
				if (itName != cdpParams.end())
					name = itName->second;
			}

			if (!handler(headers, name.c_str(), m_mapping.const_data() + pos, end - m_mapping.const_data() - pos))
				break;
		}

		if (std::memcmp(end + boundary.length(), "--", 2) == 0)
			break; // We're done
		pos = end-m_mapping.const_data() + 2;
	}
}

void MultipartFormData::parseHeaders(size_t& offset, Headers_t& out)
{
	const size_t MAX_HEADER_LENGTH = 200;

	out.clear();

	while (offset < m_mapping.size())
	{
		const char* start = m_mapping.const_data() + offset;
		void* crlf = ::memmem(start, std::min(MAX_HEADER_LENGTH, m_mapping.size() - offset),
						"\r\n", 2);

		if (!crlf)
			break;

		size_t length = static_cast<const char*>(crlf) - start;
		offset += length + 2;

		if (!length)
			break;

		const char* colon = static_cast<const char*>(std::memchr(start, ':', length));
		if (!colon)
			continue;

		std::string key(start, colon-start);

		std::transform(key.begin(), key.end(), key.begin(), [](char c) {
			return ::tolower(c);
		});

		length -= colon-start+1;
		colon++;
		
		while (length > 0 && ::isspace(*colon))
		{
			colon++;
			length--;
		}

		std::string value(colon, length);
		out.emplace(std::move(key), std::move(value));
	}
}

void MultipartFormData::parseKV(const char* input, std::string& value, Headers_t& kv)
{
	const char* pos = std::strchr(input, ';');

	if (!pos)
	{
		// No extra params
		value = input;
		return;
	}
	value.assign(input, pos-input);
	pos++; // Move past the semi-colon

	std::regex regexNoQuotes("\\s*([\\w\\-]+)=([^;]+);?");
	std::regex regexQuotes("\\s*([\\w\\-]+)=\"([^\\\"]+)\";?");

	while (true)
	{
		std::cmatch m1, m2;

		std::regex_search(pos, m1, regexQuotes);
		if (!m1.empty() && m1.position() == 0)
		{
			kv.emplace(std::string(m1[1].first, m1[1].second), std::string(m1[2].first, m1[2].second));
			pos += m1.length();
		}
		else
		{
			std::regex_search(pos, m2, regexNoQuotes);
			if (!m2.empty() && m2.position() == 0)
			{
				kv.emplace(std::string(m2[1].first, m2[1].second), std::string(m2[2].first, m2[2].second));
				pos += m2.length();
			}
			else
				break;
		}
	}
}
