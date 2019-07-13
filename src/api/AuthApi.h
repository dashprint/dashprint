#ifndef _AUTHAPI_H
#define _AUTHAPI_H
#include <memory>
#include "web/WebRouter.h"

void routeAuth(std::shared_ptr<WebRouter> router);

#endif
