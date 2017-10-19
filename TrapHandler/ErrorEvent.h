#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class ErrorEvent : public AgentEvent
		{
		public:
			std::wstring message;
			std::wstring severity;
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"error event from node: ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") sevrity: ";
				msg += severity;
				msg += L", message: ";
				msg += message;
				return msg;
			}
			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd;
				return cmd;
			}
			bool ParseTrap(const CSnmpTrap& source_trap) override 
			{
				if ((source_trap.varArgs.size() == 3 &&
					source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR) ||
					(source_trap.varArgs.size() == 4 &&
						source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR))

				{
					if (source_trap.varArgs.size() == 3)
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						message = source_trap.varArgs[1]->wcsVal;
						severity = source_trap.varArgs[2]->wcsVal;
					}
					else
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						platformId = source_trap.varArgs[1]->wcsVal;
						message = source_trap.varArgs[2]->wcsVal;
						severity = source_trap.varArgs[3]->wcsVal;
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