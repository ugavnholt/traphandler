#pragma once

#include "AgentEvent.h"

namespace traphandler
{
	namespace events
	{
		class HelloEvent : public AgentEvent
		{
		public:
			std::wstring sourceIp;
			std::wstring osFlavor = L"No OS Information - Old version of UGMon";
			std::wstring ugmonVersion = VersionPre11;
			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}
			std::wstring to_wstring() const override
			{
				std::wstring msg(L"hello from ");
				msg += hostname;
				msg += L"(";
				msg += platformId;
				msg += L") IP: ";
				msg += sourceIp;
				msg += L" - ";
				msg += osFlavor;
				msg += L", ugmon version: ";
				msg += ugmonVersion;
				return msg;
			}
			std::wstring to_cmd_str() const override 
			{
				std::wstring cmd(L"opcmsg a=UGMon o=helloMessage -node=");
				cmd += hostname;
				cmd += L" msg_t=\"host='";
				cmd += hostname;
				cmd += L"' platform='";
				cmd += platformId;
				cmd += L"' OSFlavor='";
				cmd += osFlavor;
				cmd += L"' SourceIP='";
				cmd += sourceIp;
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
					const wchar_t *pUgmonVersion = VersionPre11;
					if (source_trap.varArgs.size() == 1)
					{
						hostname = source_trap.varArgs[0]->wcsVal;
					}
					else
					{
						hostname = source_trap.varArgs[0]->wcsVal;
						platformId = source_trap.varArgs[1]->wcsVal;
						osFlavor = source_trap.varArgs[2]->wcsVal;
						ugmonVersion = source_trap.varArgs[3]->wcsVal;
					}

					if (hostname.empty() || hostname == L" ")
					{
						dmi(Facility, L"Received invalid hostname in hello message: '%s'\r\n", hostname);
					}
					else
					{
						// Format the SNMPSourceIP as a string
						wchar_t wcssip[16];
						swprintf(wcssip, 16, L"%u.%u.%u.%u",
							(source_trap.ulSourceIP & 0xff000000) >> 24,
							(source_trap.ulSourceIP & 0x00ff0000) >> 16,
							(source_trap.ulSourceIP & 0x0000ff00) >> 8,
							(source_trap.ulSourceIP & 0x000000ff)
						);
						sourceIp = wcssip;
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