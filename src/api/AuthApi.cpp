#include "AuthApi.h"

namespace
{
	void restAuthLogin(WebRequest& req, WebResponse& resp)
	{
		
	}
}

void routeAuth(std::shared_ptr<WebRouter> router)
{
	router->post("auth/login", restAuthLogin);
}

