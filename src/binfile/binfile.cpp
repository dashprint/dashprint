#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "cstring_filter.h"

namespace fs = boost::filesystem;

void writeFiles(fs::path sourceDir, const std::vector<std::string>& files, std::ostream& out);
void iterateTree(fs::path sourceDir, std::vector<std::string>& files, std::string prefix);

int main(int argc, const char** argv)
{
	if (argc <= 0)
	{
		std::cerr << "Usage: " << argv[0] << " <input-dir> <output-file>\n";
		return 1;
	}

	try
	{
		std::string sourceTree = argv[1];
		std::ofstream outputFile(argv[2]);

		if (!boost::algorithm::ends_with(sourceTree, "/"))
			sourceTree += '/';
		
		if (!outputFile.is_open())
			throw std::runtime_error("Cannot open output file!");

		std::vector<std::string> files;
		iterateTree(sourceTree, files, "");
		writeFiles(sourceTree, files, outputFile);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}

void writeFiles(fs::path sourceDir, const std::vector<std::string>& files, std::ostream& out)
{
	out << "#include <map>\n#include <string_view>\n#include <tuple>\n#include <zlib.h>\n#include \"binfile/api.h\"\n";
	out << "using namespace std::literals;\n";
	out << "static const std::map<std::string_view,std::tuple<std::string_view, uintptr_t>> fileData = {\n";

	for (const auto& file: files)
	{
		out << "{ " << '"' << file << "\"sv, std::make_tuple(\"";

		// generate compressed string
		boost::iostreams::filtering_ostreambuf fos;
		fos.push(boost::iostreams::zlib_compressor(boost::iostreams::zlib::best_compression));
		fos.push(cstring_filter<char>());
		fos.push(out);

		std::ifstream sourceFile(sourceDir / file, std::ios_base::in | std::ios_base::binary);
		if (!sourceFile.is_open())
		{
			std::stringstream ss;
			ss << "Cannot open " << file;
			throw std::runtime_error(ss.str());
		}

		boost::iostreams::copy(sourceFile, fos);

		out << "\"sv, " << fs::file_size(sourceDir / file) << ") },\n";
	}

	out << "};\n";

	// Static functions here...
	out << R"cpp(
		namespace binfile {

		std::optional<std::string_view> CompressedAsset(const char* fname)
		{
			if (*fname == '/')
				fname++;
			if (auto it = fileData.find(fname); it != fileData.end())
				return std::get<std::string_view>(it->second);

			return std::optional<std::string_view>();
		}

		std::optional<std::vector<uint8_t>> Asset(const char* fname)
		{
			if (*fname == '/')
				fname++;
				
			if (auto it = fileData.find(fname); it != fileData.end())
			{
				std::vector<uint8_t> uncompressed(std::get<uintptr_t>(it->second));
				z_stream zs;

				if (inflateInit(&zs) != Z_OK)
					return std::optional<std::vector<uint8_t>>();

				zs.next_in = (Bytef*) std::get<std::string_view>(it->second).data();
				zs.avail_in = std::get<std::string_view>(it->second).size();
				zs.next_out = uncompressed.data();
				zs.avail_out = uncompressed.size();

				if (inflate(&zs, Z_FINISH) != Z_OK)
					return std::optional<std::vector<uint8_t>>();

				inflateEnd(&zs);
				
				return std::move(uncompressed);
			}
			
			return std::optional<std::vector<uint8_t>>();
		}

		}
	)cpp";
}

void iterateTree(fs::path sourcePath, std::vector<std::string>& files, std::string prefix)
{
	fs::directory_iterator end;

	for (fs::directory_iterator it(sourcePath); it != end; it++)
	{
		if (fs::is_directory(it->path()))
		{
			iterateTree(it->path(), files, prefix + "/" + it->path().filename().generic_string());
		}
		else
		{
			std::string path = prefix + "/" + it->path().filename().generic_string();
			files.push_back(path);
		}
	}
}
