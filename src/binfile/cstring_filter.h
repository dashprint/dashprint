#ifndef _CSTRING_FILTER_H
#define _CSTRING_FILTER_H

#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <sstream>

template<typename Ch>
class cstring_filter
{
public:
	typedef Ch char_type;
	struct category : public boost::iostreams::output_filter_tag { };

	template<typename Sink>
    std::streamsize put(Sink& snk, char_type c)
    {
		const char* hex = "0123456789ABCDEF";
		char out[4] = { '\\', 'x', 0, 0 };

		out[2] = hex[(c >> 4) & 0xf];
		out[3] = hex[c & 0xf];
		
        boost::iostreams::write(snk, out, sizeof out);
        return true;
    }
};

#endif
