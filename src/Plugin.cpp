#include "Plugin.h"
#include "PluginManager.h"
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/log/trivial.hpp>
#include <stdexcept>
#include <iostream>
#include "JSValuePP.h"

Plugin::Plugin(PluginManager& pluginManager, const char* path)
: m_manager(pluginManager)
{
	m_context = ::JS_NewContext(pluginManager.runtime());
	::JS_SetContextOpaque(m_context, this);

	boost::iostreams::mapped_file_source sourceFile(path);
	if (!sourceFile.is_open())
		throw std::runtime_error("Cannot open JS file");
	
	JSValuePP funcVal = { m_context, ::JS_Eval(m_context, sourceFile.data(), sourceFile.size(), path, JS_EVAL_TYPE_GLOBAL) };

	if (funcVal.isException())
	{
		dumpException();
		return;
	}

	auto global = JSValuePP::globalObject(m_context);
	auto pluginRoot = global.property("Plugin");
	if (!pluginRoot.isObject())
		throw std::runtime_error("Plugin doesn't set 'Plugin'");

	JSValuePP exportedClasses = eval("Object.keys(Plugin)");

	assert(exportedClasses.isArray());
	for (int i = 0; ; i++)
	{
		JSValuePP clsName = exportedClasses.property(i);
		if (clsName.isUndefined())
			break;

		std::string className = clsName.toString();
		BOOST_LOG_TRIVIAL(trace) << "Exported class: " << className;

		m_classes.emplace(className, pluginRoot.property(className.c_str()));
	}
}

Plugin::~Plugin()
{
	if (m_context)
		::JS_FreeContext(m_context);
}

JSValuePP Plugin::eval(std::string_view sv)
{
	return JSValuePP{
		m_context,
		::JS_Eval(m_context, sv.data(), sv.length(), "eval", JS_EVAL_TYPE_GLOBAL)
	};
}

// Based on js_std_dump_error in quickjs-libc.c
void Plugin::dumpException()
{
	JSValuePP e = JSValuePP::currentException(m_context);
	const bool isError = e.isError();

	if (!isError)
		std::cerr << "Throw: ";
	
	// Print exception
	std::cerr << e.toString() << std::endl;

	if (isError)
	{
		auto v = e.property("stack");
		if (!v.isUndefined())
			std::cerr << v.toString() << std::endl;
	}
}
