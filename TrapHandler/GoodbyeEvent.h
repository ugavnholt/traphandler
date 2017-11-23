#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
class GoodbyeEvent : public AgentEvent
{
public:
	std::wstring ugmonVersion = VersionPre11;
	std::wstring osFlavor = L"No OS Information - Old version of UGMon";
	std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
	{
		std::wstring cmd;
		return cmd;
	}
	std::wstring to_wstring() const override
	{
		std::wstring msg(L"Goodbye from ");
		msg += hostname;
		msg += L"(";
		msg += platformId;
		msg += L")";
		return msg;
	}
	std::wstring to_cmd_str() const override 
	{
		std::wstring cmd(L"opcmsg a=UGMon o=goodbyeMessage -node=");
		cmd += hostname;
		cmd += L" msg_t=\"host='";
		cmd += L"' platform='";
		cmd += platformId;
		cmd += L"' UGMonVer='";
		cmd += ugmonVersion;
		cmd += L"'\"";
		return cmd;
	}
	bool ParseTrap(const CSnmpTrap& source_trap) override 
	{
		if ((source_trap.varArgs.size() == 1 &&
			source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR) ||
			(source_trap.varArgs.size() == 4 &&
				source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
				source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR))
		{
			ugmonVersion = VersionPre11, 
			hostname = source_trap.varArgs[0]->wcsVal;
			if (source_trap.varArgs.size() == 4)
			{
				platformId = source_trap.varArgs[1]->wcsVal;
				osFlavor = source_trap.varArgs[2]->wcsVal;
				ugmonVersion = source_trap.varArgs[3]->wcsVal;
			}
		}
		else
		{
			return false;
		}
	}
private:
};
	}
}