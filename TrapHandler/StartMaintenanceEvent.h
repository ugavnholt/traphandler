#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class StartMaintenanceEvent : public AgentEvent
		{
		public:
			std::wstring userName;
			std::wstring description;
			std::wstring duration;
			size_t durationSeconds = 0;
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"start maintenance event for node: ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") - user: ";
				msg += userName;
				msg += L", duration: ";
				msg += duration;
				msg += L", description: ";
				msg += description;
				return msg;
			}

			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd;
				return cmd;
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
						hostname = source_trap.varArgs[0]->wcsVal;
						userName = source_trap.varArgs[1]->wcsVal;
						description = source_trap.varArgs[2]->wcsVal;
						duration = source_trap.varArgs[3]->wcsVal;
					}
					else
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						platformId = source_trap.varArgs[1]->wcsVal;
						userName = source_trap.varArgs[2]->wcsVal;
						description = source_trap.varArgs[3]->wcsVal;
						duration = source_trap.varArgs[4]->wcsVal;
					}
					durationSeconds = _wtoi(duration.c_str()) * 60;
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