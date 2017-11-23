#pragma once

#include <string>
#include <regex>
#include "utils.h"

namespace traphandler
{
	namespace model
	{

// class that represents a single threshold
class Threshold
{
public:
	enum class ThreshState
	{
		normal = 0,
		util_warning = 100,
		mb_warning = 101,
		util_high = 200,
		mb_high = 201
	};
	Threshold(long Id,
		long freeMBWarn, 
		long freeMBHigh, 
		double utilWarn, 
		double utilHigh, 
		const wchar_t *warnSeverity,
		const wchar_t *highSeverity,
		std::wstring &hostExpression,
		std::wstring &volExpression,
		long minFSSize,
		long maxFSSize) :
		id(Id),
		WarnFreeMegsThresh(freeMBWarn),
		HighFreeMegsThresh(freeMBHigh),
		WarnUtilThresh(utilWarn),
		HighUtilThresh(utilHigh)
	{
		SetHostExpr(hostExpression);
		SetVolExpr(volExpression);
		SetHighSeverity(highSeverity);
		SetWarnSeverity(warnSeverity);
	}
	Threshold(const Threshold &other) 
	{
		hostExpr = other.hostExpr;
		volExpr = other.volExpr;
		WarnSevStr = other.WarnSevStr;
		HighSevStr = other.HighSevStr;
		HighFreeMegsThresh = other.HighFreeMegsThresh;
		WarnFreeMegsThresh = other.WarnFreeMegsThresh;
		HighUtilThresh = other.HighUtilThresh;
		WarnUtilThresh = other.WarnUtilThresh;
		minFSSize = other.minFSSize;
		maxFSSize = other.maxFSSize;
	}
	Threshold(Threshold &&other) 
	{
		hostExpr = std::move(other.hostExpr);
		volExpr = std::move(other.volExpr);
		WarnSevStr = std::move(other.WarnSevStr);
		HighSevStr = std::move(other.HighSevStr);
		HighFreeMegsThresh = other.HighFreeMegsThresh;
		WarnFreeMegsThresh = other.WarnFreeMegsThresh;
		HighUtilThresh = other.HighUtilThresh;
		WarnUtilThresh = other.WarnUtilThresh;
		minFSSize = other.minFSSize;
		maxFSSize = other.maxFSSize;
	}
	inline void SetHostExpr(std::wstring &host_expression)
	{
		hostExpr.assign(host_expression, std::regex::ECMAScript |
			std::regex::icase |
			std::regex::nosubs |
			std::regex::optimize);
		hostExprStr = host_expression;
	}
	inline void SetVolExpr(std::wstring &volume_expression)
	{
		volExpr.assign(volume_expression, std::regex::ECMAScript |
			std::regex::icase | 
			std::regex::nosubs |
			std::regex::optimize);
		volExprStr = volume_expression;
	}
	inline void SetHighSeverity(const wchar_t *src_string)
	{
		HighSevStr = utils::str_to_severity(src_string);
		
	}
	inline void SetWarnSeverity(const wchar_t *src_string)
	{
		WarnSevStr = utils::str_to_severity(src_string);
	}

	inline const std::wstring to_wstring() const
	{
		wchar_t buf[2048];
		swprintf_s(buf, sizeof(buf)/sizeof(buf[0]), 
			L"Threshold %s:%s warn/high mb(%I64i/%I64i) util(%.2f/%.2f) - minFSSizeMB: %I64i, maxFSSizeMB: %I64i",
			hostExprStr.c_str(), volExprStr.c_str(),
			WarnFreeMegsThresh, HighFreeMegsThresh,
			WarnUtilThresh, HighUtilThresh,
			minFSSize, maxFSSize);
		return std::wstring(buf);
	}

	inline std::string to_string() const
	{
		char buf[2048];
		sprintf_s(buf, sizeof(buf) / sizeof(buf[0]),
			"Threshold %S:%S warn/high mb(%I64i/%I64i) util(%.2f/%.2f) - minFSSizeMB: %I64i, maxFSSizeMB: %I64i",
			hostExprStr.c_str(), volExprStr.c_str(),
			WarnFreeMegsThresh, HighFreeMegsThresh,
			WarnUtilThresh, HighUtilThresh,
			minFSSize, maxFSSize);
		return std::string(buf);
	}
	
	// Checks if hostname, volume_name and file system size matches this
	// threshold
	inline bool Matches(std::wstring &hostname, std::wstring &volume_name, int64_t fsSizeMB) const
	{
		if ((minFSSize != 0 && fsSizeMB < minFSSize) ||
			(maxFSSize != 0 && fsSizeMB > maxFSSize))
		{
			return false;
		}	// fsSize match range
		if (!std::regex_match(hostname, hostExpr))
			return false;
		if (!std::regex_match(volume_name, volExpr))
			return false;
		
		return true;
	}

	// Given current usage and size of fs, determine current threshold state
	inline ThreshState GetState(int64_t fsFreeSpaceMb, int64_t fsSizeMb)
	{
		int64_t usedMegs = fsSizeMb - fsFreeSpaceMb;
		double utilPercent = (static_cast<double>(fsFreeSpaceMb)/fsSizeMb) * 100.0f;

		ThreshState utilState = ThreshState::normal;
		if (fsSizeMb == 0)
			utilState = ThreshState::normal;
		else if (utilPercent > HighUtilThresh && HighUtilThresh != 0)
			utilState = ThreshState::util_high;
		else if (utilPercent > WarnUtilThresh && WarnUtilThresh != 0)
			utilState = ThreshState::util_warning;

		ThreshState mbState = ThreshState::normal;
		if (fsSizeMb == 0)
			mbState = ThreshState::normal;
		else if (fsFreeSpaceMb < HighFreeMegsThresh && HighFreeMegsThresh != 0)
			mbState = ThreshState::mb_high;
		else if (fsFreeSpaceMb < WarnFreeMegsThresh && WarnFreeMegsThresh != 0)
			mbState = ThreshState::mb_warning;

		// return the maximum of the two states
		// level can be calculated (0=normal, 1=warning, 2=high) by dividing the int
		// value of ThreshState with 100
		return (ThreshState)std::max<int>((int)utilState, (int)mbState);
	}

	~Threshold() {}
	long id = 0;

	int64_t HighFreeMegsThresh = 0;
	int64_t WarnFreeMegsThresh = 0;

	double	HighUtilThresh = 0.0f;
	double	WarnUtilThresh = 0.0f;

	const wchar_t *WarnSevStr = L"Warning";
	const wchar_t *HighSevStr = L"Critical";

	int64_t minFSSize = 0; 
	int64_t maxFSSize = 0;
private:
	std::wregex hostExpr;
	std::wregex volExpr;
	std::wstring hostExprStr;
	std::wstring volExprStr;
};

	} // namespace model
} // namespace traphandler