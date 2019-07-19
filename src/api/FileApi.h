#ifndef _FILEAPI_H
#define _FILEAPI_H
#include "web/WebRequest.h"
#include "web/WebResponse.h"
#include "web/WebRouter.h"
#include "FileManager.h"

void routeFile(WebRouter* router, FileManager& fileManager);

#endif
