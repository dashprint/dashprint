#include "FileManager.h"
#include <fstream>
#include <stdexcept>
#include <boost/iostreams/device/mapped_file.hpp>

FileManager::FileManager(const char* directory)
: m_path(directory)
{
	boost::filesystem::create_directories(directory);
}

std::string FileManager::saveFile(const char* name, const void* contents, size_t length)
{
	boost::system::error_code ec;
	std::string safeName = name;

	std::transform(safeName.begin(), safeName.end(), safeName.begin(), [](char c) {
		if (c == '/' || c == boost::filesystem::path::preferred_separator)
			return '_';
		return c;
	});
	std::string path = getFilePath(safeName.c_str());

	// Remove the file first so that ongoing prints aren't disrupted.
	// This doesn't work on crappy OSes such as Windows.
	boost::filesystem::remove(path, ec);

	std::ofstream file(path, std::ios_base::out | std::ios_base::binary);
	if (!file.is_open())
		throw std::runtime_error("Cannot open file for writing");

	file.write(static_cast<const char*>(contents), length);

	file.flush();
	if (file.bad())
		throw std::runtime_error("Cannot write file");

	return safeName;
}

std::string FileManager::saveFile(const char* name, const char* otherfile)
{
	boost::iostreams::mapped_file mapping(otherfile, std::ios_base::in);
	if (!mapping.is_open())
		throw std::runtime_error("Cannot read file");

	return saveFile(name, mapping.const_data(), mapping.size());
}

std::string FileManager::getFilePath(const char* name)
{
	return (m_path / name).generic_string();
}

std::vector<FileManager::FileInfo> FileManager::listFiles()
{
	std::vector<FileInfo> rv;

	for (auto& p : boost::filesystem::directory_iterator(m_path))
	{
		if (boost::filesystem::is_regular_file(p))
		{
			FileInfo fi;
			fi.name = p.path().generic_string();
			fi.length = boost::filesystem::file_size(p.path());
			fi.modifiedTime = boost::filesystem::last_write_time(p.path());

			rv.push_back(fi);
		}
	}

	return rv;
}

