#pragma once

#include <string>
#include "utils.h"

namespace traphandler
{
	namespace model
	{
		const wchar_t* const StatusNormalStr = L"Normal";
		const wchar_t* const StatusSpaceLowStr = L"LowSpace";
		const wchar_t* const StatusUtilHighStr = L"HighUtil";
		const wchar_t* const StatusHBFailedStr = L"HBFailed";
		const wchar_t* const StatusDisabledStr = L"Disabled";

enum class eStatusValue
{
	Normal = 0,
	MBLow = 1,
	UtilHigh = 2,
	HBFailed = 4,
	Disabled = 128
};

class Status
{
public:
	int status = (int)eStatusValue::Normal;

	inline int getStatus() const { return status; }
	inline void setStatus(int newStatus) { status = newStatus; }
	
	inline void setFlag(eStatusValue flag) 
	{
		if (flag == eStatusValue::Normal)
			status = (int)eStatusValue::Normal;
		status = (status | ((int)flag));
	}
	inline void clearFlag(eStatusValue flag)
	{
		if (flag == eStatusValue::Normal)
			status = (int)eStatusValue::Normal;

		status = (status ^ ((int)flag));
	}
	inline bool isSet(eStatusValue flag) const
	{
		return (status | ((int)flag));
	}
	inline bool isNormal() const
	{
		return status == (int)eStatusValue::Normal;
	}
	inline void clear()
	{
		status = (int)eStatusValue::Normal;
	}
	Status &operator=(const eStatusValue newStatus)
	{
		status = (int)newStatus;
		return *this;
	}
	Status &operator=(const int &newStatus)
	{
		setStatus(newStatus);
		return *this;
	}
};

	}
}