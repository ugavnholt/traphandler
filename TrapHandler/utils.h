#pragma once

#include <string>
#include <cctype>

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
		if (wcsicmp(str, L"critical") == 0)
			return L"Critical";
		else if (wcsicmp(str, L"major") == 0)
			return L"Major";
		else if (wcsicmp(str, L"minor") == 0)
			return L"Minor";
		else if (wcsicmp(str, L"warning") == 0)
			return L"Warning";
		else if (wcsicmp(str, L"normal") == 0)
			return L"Normal";
		else
			return L"Unknown";
	}

	inline const wchar_t *str_to_severity(const char *str)
	{
		if (stricmp(str, "critical") == 0)
			return L"Critical";
		else if (stricmp(str, "major") == 0)
			return L"Major";
		else if (stricmp(str, "minor") == 0)
			return L"Minor";
		else if (stricmp(str, "warning") == 0)
			return L"Warning";
		else if (stricmp(str, "normal") == 0)
			return L"Normal";
		else
			return L"Unknown";
	}
} // namespace utils