#ifndef _CAMERAAPI_H
#define _CAMERAAPI_H
#include "../web/WebRouter.h"
#include "CameraManager.h"
#include "AuthManager.h"

void routeCamera(WebRouter* router, CameraManager& cameraManager, AuthManager& authManager);

#endif
