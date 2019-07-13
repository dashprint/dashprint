#ifndef _PLUGIN_H
#define _PLUGIN_H
#include "quickjs/quickjs.hpp"

class PluginManager;

class Plugin
{
public:
	Plugin(PluginManager& pluginManager, const char* path);
	~Plugin();
private:
	void dumpException();
private:
	PluginManager& m_manager;
	JSContext* m_context;
};

#endif
