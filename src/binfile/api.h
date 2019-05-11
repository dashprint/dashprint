#ifndef _BINFILE_API_H
#define _BINFILE_API_H
#include <string_view>
#include <vector>
#include <optional>
#include <stdint.h>

namespace binfile
{
	std::optional<std::string_view> CompressedAsset(const char* fname);
	std::optional<std::vector<uint8_t>> Asset(const char* fname);
}

#endif
