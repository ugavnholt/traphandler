#pragma once

#include <string>
#include "CSnmpTrap.h"
#include <windows.h>
#include <string>
#include "TrapHandlerModel.h"

namespace traphandler
{
	namespace events
	{

const wchar_t* const NoPlatform = L"UNKNOWN";
const wchar_t* const VersionPre11 = L"pre-1.11";
const wchar_t* const DefaultMsgGrp = L"OS_";

enum class AgentVersion
{
	pre1_13,
	post1_13
};

class AgentEvent
{
public:
	std::wstring hostname;
	std::wstring platformId = NoPlatform;
	AgentVersion agentVersion = AgentVersion::pre1_13;
	virtual std::wstring to_cmd_str() const = 0;
	virtual bool ParseTrap(const CSnmpTrap& source_trap) = 0;
	virtual std::wstring to_wstring() const = 0;
	virtual std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) = 0;
private:
};

	}
}