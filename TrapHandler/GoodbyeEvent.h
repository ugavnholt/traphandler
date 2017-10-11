#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
class GoodbyeEvent : public AgentEvent
{
public:
	std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
	{
		std::wstring cmd;
		return cmd;
	}
	std::wstring to_wstring() const override
	{
		std::wstring msg(L"GoodbyeEvent");
		return msg;
	}
	std::wstring to_cmd_str() const override {}
	bool ParseTrap(const CSnmpTrap& source_trap) override {}
private:
};
	}
}