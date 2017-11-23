#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
class DiskMetricEvent : public AgentEvent
{
public:
	std::wstring volume_name;
	int64_t fsSizeMb;
	int64_t availableSpaceMb;
	std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
	{
		double utilPercent = -1.0f;
		if (fsSizeMb > 0)
		{
			utilPercent = (double)availableSpaceMb;
			utilPercent /= fsSizeMb;
			utilPercent *= 100.0f;
			utilPercent = 100.0f - utilPercent;
		}
		std::wstring cmd;
		return cmd;
	}
	std::wstring to_cmd_str() const override 
	{

	}
	std::wstring to_wstring() const override
	{
		std::wstring msg(
			L"Disk metric sample from");
		msg += hostname;
		msg += L"(";
		msg += platformId;
		msg += L") - volume: ";
		msg += volume_name;
		msg += L", Total space: ";
		msg += std::to_wstring(fsSizeMb);
		msg += L"mb, free space: ";
		msg += std::to_wstring(availableSpaceMb);
		msg += L"mb";
		return msg;
	}
	bool ParseTrap(const CSnmpTrap& source_trap) override
	{
		if ((source_trap.varArgs.size() == 4 &&
			source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
			source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
			source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
			source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR) ||
			(source_trap.varArgs.size() == 5 &&
				source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[4]->iValueType == SNMP_TYPE_STR))
		{
			if (source_trap.varArgs.size() == 4)
			{
				platformId = NoPlatform;					// this value is not set for the old version of the trap
				hostname = source_trap.varArgs[0]->wcsVal;
				volume_name = source_trap.varArgs[1]->wcsVal;
				fsSizeMb = _wcstoi64(source_trap.varArgs[2]->wcsVal, nullptr, 10);
				availableSpaceMb = _wcstoi64(source_trap.varArgs[3]->wcsVal, nullptr, 10);
			}
			else if (source_trap.varArgs.size() == 5)
			{
				hostname = source_trap.varArgs[0]->wcsVal;
				platformId = source_trap.varArgs[1]->wcsVal;;
				volume_name = source_trap.varArgs[1]->wcsVal;
				fsSizeMb = _wcstoi64(source_trap.varArgs[2]->wcsVal, nullptr, 10);
				availableSpaceMb = _wcstoi64(source_trap.varArgs[3]->wcsVal, nullptr, 10);
			}
			else
			{
				return false;
			}
			// check for conversion errors from the _wcstoi64 calls
			if (fsSizeMb == _I64_MAX || fsSizeMb == _I64_MIN || availableSpaceMb == _I64_MAX || availableSpaceMb == _I64_MIN)
				return false;

			// calculate utilization percent
			double utilPercent = -1.0f;
			if (fsSizeMb > 0)
			{
				// double utilPercent = 100.0f - ( ((double)FreeDiskSpace/(double)TotDiskSpace) * 100.0f );
				utilPercent = (double)availableSpaceMb;
				utilPercent /= fsSizeMb;
				utilPercent *= 100.0f;
				utilPercent = 100.0f - utilPercent;
			}
		}
		else
		{
			return false;
		}
		return true;
	}
private:
};
	}
}