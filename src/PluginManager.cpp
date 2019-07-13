#include "PluginManager.h"

PluginManager::PluginManager()
{
	m_runtime = ::JS_NewRuntime();
}

PluginManager::~PluginManager()
{
	::JS_FreeRuntime(m_runtime);
}
