#ifndef _AUTHHELPERS_H
#define _AUTHHELPERS_H
#include "AuthManager.h"
#include "web/WebRouter.h"

WebRouter::filter_t checkToken(AuthManager* authManager);
WebRouter::filter_t checkOctoprintKey(AuthManager* authManager);

#endif
