#ifndef API_WEBSOCKETS_H_
#define API_WEBSOCKETS_H_
#include "web/web.h"
#include "FileManager.h"
#include "PrinterManager.h"

void routeWebSockets(std::shared_ptr<WebRouter> router, FileManager& fileManager, PrinterManager& printerManager);

#endif
