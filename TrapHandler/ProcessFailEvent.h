#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class ProcessFailEvent : public AgentEvent
		{
		public:
			std::wstring processName;
			std::wstring currentState;
			std::wstring desiredState;
			std::wstring msgGroup;
			std::wstring severity;

			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"Process state event from ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") - process: ";
				msg += processName;
				msg += L", current state: ";
				msg += currentState;
				msg += L", desired state: ";
				msg += desiredState;
				return msg;
			}
			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd;
				if (agentVersion == AgentVersion::post1_13)
					cmd = L"opcmsg a=UGMon1_13 o=processState -node=";
				else
					cmd = L"opcmsg a=UGMon o=processState -node=";
				cmd += hostname;
				cmd += L" msg_grp=";
				cmd += msgGroup;
				if (msgGroup == DefaultMsgGrp)
					cmd += platformId;
				cmd += L" sev=";
				cmd += severity;
				cmd += L" msg_t=\"host='";
				cmd += hostname;
				cmd += L"' process='";
				cmd += processName;
				cmd += L"' currentState='";
				cmd += currentState;
				cmd += L"' desiredState='";
				cmd += desiredState;
				cmd += L"' platform='";
				cmd += platformId;
				cmd += L"'\" -option currentState=";
				cmd += currentState;
				cmd += L" -option desiredState=";
				cmd += desiredState;
				return cmd;
			}
			bool ParseTrap(const CSnmpTrap& source_trap) override 
			{
				if ((snmpTrap->varArgs.size() == 4 && // version pre 1.12 trap
					snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR &&
					snmpTrap->varArgs[1]->iValueType == SNMP_TYPE_STR &&
					snmpTrap->varArgs[2]->iValueType == SNMP_TYPE_STR &&
					snmpTrap->varArgs[3]->iValueType == SNMP_TYPE_STR) ||
					(snmpTrap->varArgs.size() == 5 && // Version 1.12 trap
						snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR &&
						snmpTrap->varArgs[1]->iValueType == SNMP_TYPE_STR &&
						snmpTrap->varArgs[2]->iValueType == SNMP_TYPE_STR &&
						snmpTrap->varArgs[3]->iValueType == SNMP_TYPE_STR &&
						snmpTrap->varArgs[4]->iValueType == SNMP_TYPE_STR) ||
						(snmpTrap->varArgs.size() == 7 &&	// Version 1.13+ trap 
							snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[1]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[2]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[3]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[4]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[5]->iValueType == SNMP_TYPE_STR &&
							snmpTrap->varArgs[6]->iValueType == SNMP_TYPE_STR))
				{
					
					if (snmpTrap->varArgs.size() == 4)
					{
						hostname = snmpTrap->varArgs[0]->wcsVal;
						processName = snmpTrap->varArgs[1]->wcsVal;
						currentState = snmpTrap->varArgs[3]->wcsVal; // current and normal state was swapped in the old format
						desiredState = snmpTrap->varArgs[2]->wcsVal;

						platformId = NoPlatform;
						msgGroup = DefaultMsgGrp;
						severity = DefaultSeverity;
					}
					else if (snmpTrap->varArgs.size() == 5)
					{
						hostname = snmpTrap->varArgs[0]->wcsVal;
						platformId = snmpTrap->varArgs[1]->wcsVal;
						processName = snmpTrap->varArgs[2]->wcsVal;
						currentState = snmpTrap->varArgs[3]->wcsVal;
						desiredState = snmpTrap->varArgs[4]->wcsVal;
						msgGroup = DefaultMsgGrp;
						severity = DefaultSeverity;
					}
					else if (snmpTrap->varArgs.size() == 7)
					{
						agentVersion = AgentVersion::post1_13;
						hostname = snmpTrap->varArgs[0]->wcsVal;
						platformId = snmpTrap->varArgs[1]->wcsVal;
						processName = snmpTrap->varArgs[2]->wcsVal;
						currentState = snmpTrap->varArgs[3]->wcsVal;
						desiredState = snmpTrap->varArgs[4]->wcsVal;
						msgGroup = snmpTrap->varArgs[5]->wcsVal;
						severity = utils::str_to_severity(snmpTrap->varArgs[6]->wcsVal);
					}
				}
				else
				{ // incorrect message
					return false;
				}

				return true;
			}
		private:
		};
	}
}