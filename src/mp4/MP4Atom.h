#ifndef _MP4ATOM_H
#define _MP4ATOM_H
#include <string>
#include <string_view>
#include <endian.h>
#include <cstring>

class MP4Atom
{
public:
	MP4Atom(uint32_t fourcc)
	{
		const uint32_t length = htobe32(sizeof(uint32_t)*2);
		m_data.append(reinterpret_cast<const char*>(&length), sizeof(length));
		m_data.append(reinterpret_cast<const char*>(&fourcc), sizeof(fourcc));
	}

	MP4Atom& operator+=(std::string_view sv)
	{
		m_data += sv;
		return *this;
	}

	MP4Atom& operator+=(const MP4Atom& subatom)
	{
		m_data += subatom.data();
		updateLength();
		return *this;
	}

	MP4Atom& append(uint64_t v)
	{
		v = htobe64(v);
		m_data.append(reinterpret_cast<const char*>(&v), sizeof(v));
		return *this;
	}

	MP4Atom& append(uint32_t v)
	{
		v = htobe32(v);
		m_data.append(reinterpret_cast<const char*>(&v), sizeof(v));
		return *this;
	}

	MP4Atom& append(uint16_t v)
	{
		v = htobe16(v);
		m_data.append(reinterpret_cast<const char*>(&v), sizeof(v));
		return *this;
	}

	MP4Atom& append(std::string_view sv)
	{
		m_data += sv;
		return *this;
	}

	MP4Atom& append(const char* str)
	{
		m_data += str;
		return *this;
	}

	MP4Atom& appendWithNul(const char* str)
	{
		m_data.append(str, std::strlen(str)+1);
		return *this;
	}

	MP4Atom& append(size_t n, char c)
	{
		m_data.append(n, c);
		return *this;
	}

	const std::string& data() const { return m_data; }
	size_t length() const { return m_data.length(); }
private:
	void updateLength()
	{
		const uint32_t length = htobe32(m_data.length());
		m_data.replace(0, sizeof(length), reinterpret_cast<const char*>(&length), sizeof(length));
	}
private:
	std::string m_data;
};

#endif
