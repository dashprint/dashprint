#ifndef _AUTHAPI_H
#define _AUTHAPI_H
#include <memory>
#include "web/WebRouter.h"
#include "AuthManager.h"

void routeAuth(WebRouter* router, AuthManager& authManager);

#endif
