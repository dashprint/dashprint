#include "FileManager.h"
#include <fstream>
#include <stdexcept>
#include <boost/iostreams/device/mapped_file.hpp>

FileManager::FileManager(std::string_view directory)
: m_path(std::string(directory))
{
	boost::filesystem::create_directories(m_path);
}

std::string FileManager::saveFile(std::string_view name, const void* contents, size_t length)
{
	boost::system::error_code ec;
	std::string safeName = std::string(name);

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

std::string FileManager::saveFile(std::string_view name, std::string_view otherfile)
{
	boost::iostreams::mapped_file mapping(std::string(otherfile), std::ios_base::in);
	if (!mapping.is_open())
		throw std::runtime_error("Cannot read file");

	return saveFile(name, mapping.const_data(), mapping.size());
}

std::string FileManager::getFilePath(std::string_view name)
{
	return (m_path / std::string(name)).generic_string();
}

std::vector<FileManager::FileInfo> FileManager::listFiles()
{
	std::vector<FileInfo> rv;

	for (auto& p : boost::filesystem::directory_iterator(m_path))
	{
		if (boost::filesystem::is_regular_file(p))
		{
			FileInfo fi;
			fi.name = p.path().filename().generic_string();
			fi.length = boost::filesystem::file_size(p.path());
			fi.modifiedTime = boost::filesystem::last_write_time(p.path());

			rv.push_back(fi);
		}
	}

	// Last modified first
	std::sort(rv.begin(), rv.end(), [](const FileInfo& f1, const FileInfo& f2) {
		return f1.modifiedTime > f2.modifiedTime;
	});

	return rv;
}

