#ifndef _BINFILE_API_H
#define _BINFILE_API_H
#include <string_view>
#include <string>
#include <optional>
#include <stdint.h>

namespace binfile
{
	// Returns a gzipped asset
	std::optional<std::string_view> compressedAsset(std::string_view fname);

	// Returns an asset in plain text
	std::optional<std::string> asset(std::string_view fname);
}

#endif
