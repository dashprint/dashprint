#ifndef OCTOPRINTRESTAPI_H
#define OCTOPRINTRESTAPI_H
#include "web/web.h"
#include "FileManager.h"
#include "PrinterManager.h"
#include "AuthManager.h"

void routeOctoprintRest(WebRouter* router, FileManager& fileManager, PrinterManager& printerManager, AuthManager& authManager);

#endif
