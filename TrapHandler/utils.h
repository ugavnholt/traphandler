#pragma once

#include "stdafx.h"
#include <string>
#include <cctype>

#ifndef __DEBUG_PRINT
#define __DEBUG_PRINT
// #define print(x,...) if(bDebug) {_print(x,__VA_ARGS__);}
#define print(x,z,...) pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);
inline void _print(const wchar_t *format, ...)
{
	if (!bDebug)
		return;

	va_list argList;
	va_start(argList, format);

	vwprintf(format, argList);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug macros
// z = message
#define dd(x,z,...)	pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);
#define dn(x,z,...)	pTrace->SendEvent(LOG_NORMAL, x, z, __VA_ARGS__);
#define dw(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);
#define dmi(x,z,...)	pTrace->SendEvent(LOG_MINOR, x, z, __VA_ARGS__);
#define dma(x,z,...)	pTrace->SendEvent(LOG_MAJOR, x, z, __VA_ARGS__);
#define dc(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);


namespace utils
{
	bool iequals(const std::string& str1, const std::string& str2);
	bool iequals(const std::wstring& str1, const std::wstring& str2);
	const wchar_t *str_to_severity(std::string const &str);
	const wchar_t *str_to_severity(std::wstring const &str);
	const wchar_t *str_to_severity(const wchar_t *str);
	const wchar_t *str_to_severity(const char *str);
	void remove_str_quotes_and_back_spaces(wchar_t *str);

	struct iequal
	{
		bool operator()(int c1, int c2) const
		{

			return std::toupper(c1) == std::toupper(c2);
		}
	};

	inline void remove_str_quotes_and_back_spaces(wchar_t *str)
	{
		while (*str != L'\0')
		{
			if (*str == L'"') *str = L'\'';
			else if (*str == L'\\') *str = L'/';
			str++;
		}
	}

	inline bool iequals(const std::string& str1, const std::string& str2)
	{
		return std::equal(str1.begin(), str1.end(), str2.begin(), iequal());
	}
	inline bool iequals(const std::wstring& str1, const std::wstring& str2)
	{
		return std::equal(str1.begin(), str1.end(), str2.begin(), iequal());
	}

	inline const wchar_t *str_to_severity(std::string const &str)
	{
		if (utils::iequals(str, std::string("critical")))
			return L"Critical";
		else if (utils::iequals(str, std::string("major")))
			return L"Major";
		else if (utils::iequals(str, std::string("minor")))
			return L"Minor";
		else if (utils::iequals(str, std::string("warning")))
			return L"Warning";
		else if (utils::iequals(str, std::string("normal")))
			return L"Normal";
		else
			return L"Unknown";
	}

	inline const wchar_t *str_to_severity(std::wstring const &str)
	{
		if (utils::iequals(str, std::wstring(L"critical")))
			return L"Critical";
		else if (utils::iequals(str, std::wstring(L"major")))
			return L"Major";
		else if (utils::iequals(str, std::wstring(L"minor")))
			return L"Minor";
		else if (utils::iequals(str, std::wstring(L"warning")))
			return L"Warning";
		else if (utils::iequals(str, std::wstring(L"normal")))
			return L"Normal";
		else
			return L"Unknown";
	}

	inline const wchar_t *str_to_severity(const wchar_t *str)
	{
		if (_wcsicmp(str, L"critical") == 0)
			return L"Critical";
		else if (_wcsicmp(str, L"major") == 0)
			return L"Major";
		else if (_wcsicmp(str, L"minor") == 0)
			return L"Minor";
		else if (_wcsicmp(str, L"warning") == 0)
			return L"Warning";
		else if (_wcsicmp(str, L"normal") == 0)
			return L"Normal";
		else
			return L"Unknown";
	}

	inline const wchar_t *str_to_severity(const char *str)
	{
		if (_stricmp(str, "critical") == 0)
			return L"Critical";
		else if (_stricmp(str, "major") == 0)
			return L"Major";
		else if (_stricmp(str, "minor") == 0)
			return L"Minor";
		else if (_stricmp(str, "warning") == 0)
			return L"Warning";
		else if (_stricmp(str, "normal") == 0)
			return L"Normal";
		else
			return L"Unknown";
	}
} // namespace utils