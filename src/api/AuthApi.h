#ifndef _AUTHAPI_H
#define _AUTHAPI_H
#include <memory>
#include "web/WebRouter.h"
#include "AuthManager.h"

void routeAuth(std::shared_ptr<WebRouter> router, AuthManager& authManager);

#endif
