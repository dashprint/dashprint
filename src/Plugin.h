#ifndef _PLUGIN_H
#define _PLUGIN_H
#include "quickjs/quickjs.hpp"
#include "JSValuePP.h"
#include <map>
#include <string>

class PluginManager;

class Plugin
{
public:
	Plugin(PluginManager& pluginManager, const char* path);
	~Plugin();
private:
	void dumpException();
	JSValuePP eval(std::string_view sv);
private:
	PluginManager& m_manager;
	JSContext* m_context;
	std::map<std::string, JSValuePP> m_classes;
};

#endif
