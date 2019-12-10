#include "FileApi.h"
#include "web/WebServer.h"

namespace
{
	void restListFiles(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::vector<FileManager::FileInfo> files = fileManager->listFiles();
		nlohmann::json result = nlohmann::json::array();

		for (const FileManager::FileInfo& file : files)
		{
			nlohmann::json ff = nlohmann::json::object();

			ff["name"] = file.name;
			ff["length"] = file.length;
			ff["mtime"] = file.modifiedTime;

			result.push_back(ff);
		}

		resp.send(result, WebResponse::http_status::ok);
	}

	void restDeleteFile(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::string name = WebRequest::urlDecode(req.pathParam(1));
		if (!fileManager->deleteFile(name.c_str()))
			throw WebErrors::not_found("Cannot delete file");
		
		resp.send(WebResponse::http_status::no_content);
	}

	void restUploadFile(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::string name = WebRequest::urlDecode(req.pathParam(1));
		
		if (req.hasRequestFile())
			name = fileManager->saveFile(name.c_str(), req.requestFile().c_str());
		else
			name = fileManager->saveFile(name.c_str(), req.request().body().c_str(), req.request().body().length());

		std::string realUrl = req.baseUrl() + "/api/v1/files/" + name;
		resp.set(WebResponse::http_header::location, realUrl);
		resp.send(boost::beast::http::status::created);
	}

	void restDownloadFile(WebRequest& req, WebResponse& resp, FileManager* fileManager)
	{
		std::string name = WebRequest::urlDecode(req.pathParam(1));
		std::string path = fileManager->getFilePath(name.c_str());

		resp.sendFile(path.c_str());
	}
}

void routeFile(WebRouter* router, FileManager& fileManager)
{
	router->post("files/([^/]+)", restUploadFile, &fileManager);
	router->get("files/([^/]+)", restDownloadFile, &fileManager);
	router->delete_("files/([^/]+)", restDeleteFile, &fileManager);
	router->get("files", restListFiles, &fileManager);
}