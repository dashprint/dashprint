#ifndef _FILEMANAGER_H
#define _FILEMANAGER_H
#include <boost/filesystem.hpp>
#include <string>
#include <string_view>
#include <ctime>
#include <vector>

class FileManager
{
public:
	FileManager(std::string_view directory);

	std::string saveFile(std::string_view, const void* contents, size_t length);
	std::string saveFile(std::string_view, std::string_view otherfile);
	std::string getFilePath(std::string_view name);

	struct FileInfo
	{
		std::string name;
		size_t length;
		time_t modifiedTime;
	};
	std::vector<FileInfo> listFiles();
private:
	boost::filesystem::path m_path;
};

#endif
