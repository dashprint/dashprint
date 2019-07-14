#include "PluginManager.h"
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/log/trivial.hpp>
#include "Plugin.h"

PluginManager::PluginManager()
{
	m_runtime = ::JS_NewRuntime();
	loadPlugins();
}

PluginManager::~PluginManager()
{
	::JS_FreeRuntime(m_runtime);
}

void PluginManager::loadPlugins()
{
	boost::filesystem::path pluginDir(::getenv("HOME"));

	pluginDir /= ".local/share/dashprint/plugins";

	for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(pluginDir), {}))
	{
		const auto& path = entry.path();
		if (boost::filesystem::extension(path) == ".js")
			loadPlugin(path.generic_string().c_str());
	}
}

void PluginManager::loadPlugin(const char* path)
{
	try
	{
		BOOST_LOG_TRIVIAL(info) << "Loading plugin " << path;
		Plugin* plugin = new Plugin(*this, path);
	}
	catch (const std::exception& e)
	{
		BOOST_LOG_TRIVIAL(error) << "Plugin " << path << " failed to load: " << e.what();
	}
}
