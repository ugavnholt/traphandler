#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class ServiceFailEvent : public AgentEvent
		{
		public:
			std::wstring serviceName;
			std::wstring currentState;
			std::wstring desiredState;
			std::wstring serviceLabel;
			std::wstring msgGroup;
			std::wstring msgSeverity;
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"service state event from ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") - service: ";
				msg += serviceName;
				msg += L", currentstate: ";
				msg += currentState;
				msg += L", desiredstate: ";
				msg += desiredState;
				msg += L", serviceLabel: ";
				msg += serviceLabel;
				return msg;
			}
			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd = L"opcmsg o=serviceState -node=";
				cmd += hostname;
				if (agentVersion >= AgentVersion::post1_13)
					cmd += L" a=UGMon1_13 msg_grp=";
				else
					cmd += L" a=UGMon msg_grp=";
				cmd += msgGroup;
				if (msgGroup == DefaultMsgGrp)
					cmd += platformId;
				cmd += L" sev=";
				cmd += msgSeverity;
				cmd += L"msg_t=\"host='";
				cmd += hostname;
				cmd += L"' service='";
				cmd += serviceName;
				cmd += L"' currentState='";
				cmd += currentState;
				cmd += L"' desiredState='";
				cmd += desiredState;
				cmd += L"' platform='";
				cmd += platformId;
				cmd += L"'\" -option currentstate=";
				cmd += currentState;
				cmd += L" -option desiredState=";
				cmd += desiredState;
				cmd += L" -option label=\"";
				cmd += serviceLabel;
				cmd += L"\"";

				return cmd;
			}
			bool ParseTrap(const CSnmpTrap &source_trap) override 
			{
				if ((source_trap.varArgs.size() == 5 &&	// version pre 1.12 trap
					source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR &&
					source_trap.varArgs[4]->iValueType == SNMP_TYPE_STR) ||
					(source_trap.varArgs.size() == 6 &&	// Version 1.12 trap 
						source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[4]->iValueType == SNMP_TYPE_STR &&
						source_trap.varArgs[5]->iValueType == SNMP_TYPE_STR) ||
						(source_trap.varArgs.size() == 8 && // Version 1.13+ trap 
							source_trap.varArgs[0]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[1]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[2]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[3]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[4]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[5]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[6]->iValueType == SNMP_TYPE_STR &&
							source_trap.varArgs[7]->iValueType == SNMP_TYPE_STR
							))
				{
					if (source_trap.varArgs.size() == 5)
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						serviceName = source_trap.varArgs[1]->wcsVal;
						currentState = source_trap.varArgs[3]->wcsVal;
						desiredState = source_trap.varArgs[2]->wcsVal;   // current and normal state was swapped in the old format
						serviceLabel = source_trap.varArgs[4]->wcsVal;
						msgGroup = DefaultMsgGrp;
						msgSeverity = DefaultSeverity;
					}
					else if (source_trap.varArgs.size() == 6)
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						platformId = source_trap.varArgs[1]->wcsVal;
						serviceName = source_trap.varArgs[2]->wcsVal;
						currentState = source_trap.varArgs[3]->wcsVal;
						desiredState = source_trap.varArgs[4]->wcsVal;
						serviceLabel = source_trap.varArgs[5]->wcsVal;
						msgGroup = DefaultMsgGrp;
						msgSeverity = DefaultSeverity;
					}
					else if (source_trap.varArgs.size() == 8)
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						platformId = source_trap.varArgs[1]->wcsVal;
						serviceName = source_trap.varArgs[2]->wcsVal;
						currentState = source_trap.varArgs[3]->wcsVal;
						desiredState = source_trap.varArgs[4]->wcsVal;
						serviceLabel = source_trap.varArgs[5]->wcsVal;
						msgGroup = source_trap.varArgs[6]->wcsVal;
						msgSeverity = utils::str_to_severity(source_trap.varArgs[7]->wcsVal);
					}
					else
					{
						return false;
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