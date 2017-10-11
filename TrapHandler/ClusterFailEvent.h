#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class ClusterFailEvent : public AgentEvent
		{
		public:
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"ClusterFailEvent");
				return msg;
			}
			std::wstring to_cmd_str() const override {}
			bool ParseTrap(const CSnmpTrap& source_trap) override {}
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
		private:
		};
	}
}