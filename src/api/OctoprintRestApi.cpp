#include "OctoprintRestApi.h"

namespace
{
	void octoprintGetVersion(WebRequest& req, WebResponse& resp)
	{
		resp.send("{\"api\": \"0.1\", \"server\": \"1.0.0-dashprint\"}", "application/json");
	}

	void octoprintUploadGcode(WebRequest& req, WebResponse& resp, PrinterManager* printerManager, FileManager* fileManager)
	{
		// TODO: Support "path" parameter

		std::unique_ptr<MultipartFormData> mp(req.multipartBody());
		const void* fileData = nullptr;
		size_t fileSize;
		std::string fileName;
		bool print = false;

		if (!mp)
			throw WebErrors::bad_request("multipart data expected");

		mp->parse([&](const MultipartFormData::Headers_t& headers, const char* name, const void* data, uint64_t length) {
			if (std::strcmp(name, "print") == 0)
			{
				if (length == 4 && std::memcmp(data, "true", 4) == 0)
					print = true;
			}
			else if (std::strcmp(name, "file") == 0)
			{
				auto itCDP = headers.find("content-disposition");
				if (itCDP != headers.end())
				{
					std::string value;
					MultipartFormData::Headers_t params;

					MultipartFormData::parseKV(itCDP->second.c_str(), value, params);
					auto it = params.find("filename");

					if (it != params.end())
						fileName = it->second;
				}

				fileData = data;
				fileSize = length;
			}
			return true;
		});

		if (!fileData || fileName.empty())
			throw WebErrors::bad_request("missing fields");

		fileName = fileManager->saveFile(fileName.c_str(), fileData, fileSize);

		nlohmann::json result = nlohmann::json::object();
		result["done"] = true;

		nlohmann::json local = nlohmann::json::object();
		local["name"] = fileName;
		local["origin"] = "local";
		local["path"] = fileName;

		nlohmann::json refs = {
			"download", req.baseUrl() + "/api/v1/files/" + fileName,
			"resource", req.baseUrl() + "/api/files/local/" + fileName
		};

		local["refs"] = refs;
		result["files"] = { "local", local };

		// TODO: Add location header!
		resp.send(result, WebResponse::http_status::created);
	}

}

void routeOctoprintRest(std::shared_ptr<WebRouter> router, FileManager& fileManager, PrinterManager& printerManager)
{
	router->get("version", octoprintGetVersion);
	router->post("files/local", octoprintUploadGcode, &printerManager, &fileManager);
}
