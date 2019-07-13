#ifndef _JSVALUEPP_H
#define _JSVALUEPP_H
#include "quickjs/quickjs.hpp"
#include <string_view>
#include <string>

class JSValuePP
{
public:
	JSValuePP(JSContext* context)
	: m_context(context), m_value(JS_UNDEFINED)
	{
	}

	JSValuePP(JSContext* context, JSValue v)
	: m_context(context), m_value(v)
	{
	}
	JSValuePP(const JSValuePP& that)
	{
		m_context = that.m_context;
		m_value = ::JS_DupValue(m_context, that.m_value);
	}
	JSValuePP(JSValuePP&& that)
	{
		m_context = that.m_context;
		m_value = that.m_value;
	}
	~JSValuePP()
	{
		::JS_FreeValue(m_context, m_value);
	}

	JSValuePP(JSContext* context, bool boolval)
	: m_context(context)
	{
		m_value = ::JS_NewBool(m_context, boolval);
	}

	JSValuePP(JSContext* context, int intval)
	: m_context(context)
	{
		m_value = ::JS_NewInt32(m_context, intval);
	}

	JSValuePP(JSContext* context, double doubleval)
	: m_context(context)
	{
		m_value = ::JS_NewFloat64(m_context, doubleval);
	}

	JSValuePP(JSContext* context, std::string_view sv)
	: m_context(context)
	{
		m_value = ::JS_NewStringLen(m_context, sv.data(), sv.length());
	}

	JSValuePP& operator=(const JSValuePP& that)
	{
		::JS_FreeValue(m_context, m_value);

		m_context = that.m_context;
		m_value = ::JS_DupValue(m_context, that.m_value);

		return *this;
	}

	JSValuePP& operator=(const JSValuePP&& that)
	{
		::JS_FreeValue(m_context, m_value);

		m_context = that.m_context;
		m_value = that.m_value;

		return *this;
	}

	bool isError() const
	{
		return ::JS_IsError(m_context, m_value);
	}

	bool isFunction() const
	{
		return ::JS_IsFunction(m_context, m_value);
	}

	bool isArray() const
	{
		return ::JS_IsArray(m_context, m_value);
	}

	bool isUndefined() const
	{
		return ::JS_IsUndefined(m_value);
	}

	bool isNull() const
	{
		return ::JS_IsNull(m_value);
	}

	bool isBool() const
	{
		return ::JS_IsBool(m_value);
	}

	bool isObject() const
	{
		return ::JS_IsObject(m_value);
	}

	bool isString() const
	{
		return ::JS_IsString(m_value);
	}

	bool isInteger() const
	{
		return ::JS_IsInteger(m_value);
	}

	bool isBigFloat() const
	{
		return ::JS_IsBigFloat(m_value);
	}

	bool isNumber() const
	{
		return ::JS_IsNumber(m_value);
	}

	bool isException() const
	{
		return ::JS_IsException(m_value);
	}

	JSValuePP&& property(const char* propName)
	{
		JSValuePP v = { m_context, ::JS_GetPropertyStr(m_context, m_value, propName) };
		return std::move(v);
	}

	JSValuePP&& property(uint32_t index)
	{
		JSValuePP v = { m_context, ::JS_GetPropertyUint32(m_context, m_value, index) };
		return std::move(v);
	}

	std::string toString()
	{
		JSValuePP v(m_context, ::JS_ToString(m_context, m_value));
		const char* str = ::JS_ToCString(m_context, v.m_value);
		std::string rv = str;

		::JS_FreeCString(m_context, str);
		return rv;
	}

	static JSValuePP&& null(JSContext* context)
	{
		JSValuePP v(context);
		v.m_value = JS_NULL;
		return std::move(v);
	}

	static JSValuePP&& currentException(JSContext* context)
	{
		JSValuePP v(context, ::JS_GetException(context));
		return std::move(v);
	}
private:
	mutable JSContext* m_context;
	mutable JSValue m_value;
};

#endif
