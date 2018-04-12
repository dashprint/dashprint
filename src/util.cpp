//
// Created by lubos on 11.4.18.
//

#include <iconv.h>
#include <ctype.h>
#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

extern boost::property_tree::ptree g_config;

std::string urlSafeString(const char* str, const char* fallback)
{
	iconv_t convertor = ::iconv_open("ASCII//TRANSLIT//IGNORE", "UTF-8");

	size_t inbytesleft = strlen(str);
	size_t outbytesleft = inbytesleft * 3;
	std::unique_ptr<char[]> ascii(new char[outbytesleft]);

	char* inbuf = const_cast<char*>(str);
	char* outbuf = ascii.get();

	size_t res = ::iconv(convertor, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

	::iconv_close(convertor);

	if (res == size_t(-1))
		return fallback;

	std::string rv(ascii.get(), outbuf - ascii.get());
	std::transform(rv.begin(), rv.end(), rv.begin(), [](char c) {
		c = ::tolower(c);

		return ::isalnum(c) ? c : '_';
	});

	boost::replace_all(rv, "__", "_");
	boost::replace_all(rv, "__", "_");

	return rv;
}

void loadConfig()
{
	boost::filesystem::path configPath(::getenv("HOME"));

	configPath /= ".config/pi3printrc";

	try
	{
		boost::property_tree::info_parser::read_info(configPath.string(), g_config);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Failed to load configuration: " << e.what();
	}
}

void saveConfig()
{
	boost::filesystem::path configPath(::getenv("HOME"));

	configPath /= ".config/pi3printrc";

	try
	{
		boost::property_tree::write_info(configPath.string(), g_config);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(warning) << "Failed to save configuration: " << e.what();
	}
}
