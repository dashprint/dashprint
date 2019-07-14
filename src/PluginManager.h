#ifndef _PLUGINMANAGER_H
#define _PLUGINMANAGER_H
#include "quickjs/quickjs.hpp"

class PluginManager
{
public:
	PluginManager();
	~PluginManager();

	void loadPlugin(const char* path);
private:
	void loadPlugins();
protected:
	JSRuntime* runtime() { return m_runtime; }
private:
	JSRuntime* m_runtime;

	friend class Plugin;
};

#endif
