#ifndef API_WEBSOCKETS_H_
#define API_WEBSOCKETS_H_
#include "web/web.h"
#include "FileManager.h"
#include "PrinterManager.h"
#include "AuthManager.h"

void routeWebSockets(WebRouter* router, FileManager& fileManager, PrinterManager& printerManager, AuthManager& authManager);

#endif
