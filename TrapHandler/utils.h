#pragma once

#include <string>
#include <cctype>
#include <windows.h>


namespace utils
{
	bool iequals(const std::string& str1, const std::string& str2);
	bool iequals(const std::wstring& str1, const std::wstring& str2);
	const wchar_t *str_to_severity(std::string const &str);
	const wchar_t *str_to_severity(std::wstring const &str);
	const wchar_t *str_to_severity(const wchar_t *str);
	const wchar_t *str_to_severity(const char *str);
	void remove_str_quotes_and_back_spaces(wchar_t *str);
	uint64_t fttoull(const FILETIME &ft);
	bool replace(std::string& str, const std::string& from, const std::string& to);
	bool replace(std::wstring& str, const std::wstring& from, const std::wstring& to);
	void replaceAll(std::string& str, const std::string& from, const std::string& to);
	void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
	uint64_t unixtime2filetime(uint64_t unix_time);

	inline uint64_t unixtime2filetime(uint64_t unix_time)
	{
		uint64_t file_time = unix_time;
		file_time *= FTCLICKSQERSEC;
		file_time += 116444736000000000;
		return file_time;
	}

	inline void replaceAll(std::string& str, const std::string& from, const std::string& to) 
	{
		if (from.empty())
			return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	inline void replaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
	{
		if (from.empty())
			return;
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
		}
	}

	inline bool replace(std::string& str, const std::string& from, const std::string& to) 
	{
		size_t start_pos = str.find(from);
		if (start_pos == std::string::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	inline bool replace(std::wstring& str, const std::wstring& from, const std::wstring& to)
	{
		size_t start_pos = str.find(from);
		if (start_pos == std::wstring::npos)
			return false;
		str.replace(start_pos, from.length(), to);
		return true;
	}

	inline uint64_t fttoull(const FILETIME &ft)
	{
		uint64_t retVal = ft.dwHighDateTime;
		retVal = retVal << 32;
		return  (retVal += ft.dwLowDateTime);
	}

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