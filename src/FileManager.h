#ifndef _FILEMANAGER_H
#define _FILEMANAGER_H
#include <boost/filesystem.hpp>
#include <string>
#include <ctime>
#include <vector>

class FileManager
{
public:
	FileManager(const char* directory);

	std::string saveFile(const char* name, const void* contents, size_t length);
	std::string saveFile(const char* name, const char* otherfile);
	std::string getFilePath(const char* name);

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
