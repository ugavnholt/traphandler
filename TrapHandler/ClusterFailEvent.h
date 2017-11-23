#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class ClusterFailEvent : public AgentEvent
		{
		public:
			std::wstring resourceGroup;
			std::wstring clusterName;
			std::wstring state;
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"cluster state event from ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") - resource group: ";
				msg += resourceGroup;
				msg += L", cluster name: ";
				msg += clusterName;
				msg += L", state: ";
				msg += state;
				return msg;
			}
			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd;
				return cmd;
			}
			bool ParseTrap(const CSnmpTrap& source_trap) override 
			{
				if ((source_trap.varArgs.size() == 5 &&
					source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[4]->iValueType == SNMP_TYPE_STR))
				{
					hostname = source_trap.varArgs[0]->wcsVal;
					platformId = source_trap.varArgs[1]->wcsVal;
					resourceGroup = source_trap.varArgs[2]->wcsVal;
					clusterName = source_trap.varArgs[3]->wcsVal;
					state = source_trap.varArgs[4]->wcsVal;
				}
				else
				{
					return false;
				}
			}
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
		private:
		};
	}
}