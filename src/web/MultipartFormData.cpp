#include "MultipartFormData.h"
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <regex>
#include <iostream>
#include <ctype.h>

MultipartFormData::MultipartFormData(const char* filename, const char* boundary)
: m_contents(nullptr)
{
	m_mapping.open(filename, std::ios_base::in);
	if (!m_mapping.is_open())
	{
		std::stringstream ss;
		ss << "Cannot open " << filename;
		throw std::runtime_error(ss.str());
	}

	if (boundary)
	{
		m_boundary = "--";
		m_boundary += boundary;
	}
}

MultipartFormData::MultipartFormData(const std::string* contents, const char* boundary)
: m_contents(contents)
{
	if (boundary)
	{
		m_boundary = "--";
		m_boundary += boundary;
	}
}

MultipartFormData::~MultipartFormData()
{

}

const char* MultipartFormData::data() const
{
	if (m_contents)
		return m_contents->data();
	else
		return m_mapping.const_data();
}

size_t MultipartFormData::length() const
{
	if (m_contents)
		return m_contents->length();
	else
		return m_mapping.size();
}

void MultipartFormData::parse(FieldHandler_t handler)
{
	size_t pos = 0;
	std::string boundary;

	while (pos < length())
	{
		Headers_t headers;
		bool contentSection = true;

		if (!boundary.empty() || m_boundary.empty())
			parseHeaders(pos, headers);
		else if (boundary.empty() && !m_boundary.empty())
			boundary = m_boundary;

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

		const char* bdr = boundary.c_str();
		const char* end = static_cast<const char*>(::memmem(data() + pos, length() - pos, boundary.c_str(), boundary.length()));
		if (!end || end-data() >= length()-2)
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

			if (!handler(headers, name.c_str(), data() + pos, end - data() - pos))
				break;
		}

		if (std::memcmp(end + boundary.length(), "--", 2) == 0)
			break; // We're done
		pos = end-data() + 2;
	}
}

void MultipartFormData::parseHeaders(size_t& offset, Headers_t& out)
{
	const size_t MAX_HEADER_LENGTH = 200;

	out.clear();

	while (offset < length())
	{
		const char* start = data() + offset;
		void* crlf = ::memmem(start, std::min(MAX_HEADER_LENGTH, length() - offset),
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

void MultipartFormData::parseKV(std::string_view input, std::string& value, Headers_t& kv)
{
	auto pos = input.find(';');
	if (pos == std::string_view::npos)
	{
		// No extra params
		value = input;
		return;
	}

	value.assign(input.substr(0, pos));
	input.remove_prefix(pos+1); // Move past the semi-colon

	std::regex regexNoQuotes("\\s*([\\w\\-]+)=([^;]+);?");
	std::regex regexQuotes("\\s*([\\w\\-]+)=\"([^\\\"]+)\";?");

	while (true)
	{
		std::cmatch m1, m2;

		std::regex_search(input.begin(), input.end(), m1, regexQuotes);
		if (!m1.empty() && m1.position() == 0)
		{
			kv.emplace(std::string(m1[1].first, m1[1].second), std::string(m1[2].first, m1[2].second));
			input.remove_prefix(m1.length());
		}
		else
		{
			std::regex_search(input.begin(), input.end(), m2, regexNoQuotes);
			if (!m2.empty() && m2.position() == 0)
			{
				kv.emplace(std::string(m2[1].first, m2[1].second), std::string(m2[2].first, m2[2].second));
				input.remove_prefix(m2.length());
			}
			else
				break;
		}
	}
}
