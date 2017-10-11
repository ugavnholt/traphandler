#pragma once

#include "AgentEvent.h"
#include <vector>
#include "utils.h"

namespace traphandler
{
	namespace events
	{
		class GenericEvent : public AgentEvent
		{
		public:
			std::wstring oid;
			std::wstring hostname;
			std::wstring source_ip;
			std::vector<std::wstring> args;

			std::wstring ProcessTrap(traphandler::model::TrapHandlerModel& model) override
			{
				std::wstring cmd;
				return cmd;
			}

			std::wstring to_wstring() const override
			{
				std::wstring msg(L"GenericEvent");
				return msg;
			}
			std::wstring to_cmd_str() const override
			{
				std::wstring cmd = L"opcmsg a=UGMon o=Generic_Trap msg_t=\"Generic trap received: ";
				cmd += oid;
				cmd += L"\" -node=" + hostname;
				cmd += L" -option trapOid=" + oid;
				cmd += L" -option sourceIP=" + source_ip;

				for (size_t i = 0; i < args.size(); i++)
				{
					cmd += L" -option ARG" + i;
					cmd += L"=\"" + args[i] + L"\"";
				}
				return cmd;
			}
			bool ParseTrap(const CSnmpTrap& source_trap) override 
			{
				if (source_trap.varArgs.size() == 0)
					return false;
				oid = (wchar_t*)source_trap.TrapOid;
				hostname = (wchar_t*)source_trap.varArgs[0]->wcsVal;
				wchar_t wcssip[16];
				swprintf(wcssip, 16, L"%u.%u.%u.%u",
					(source_trap.ulSourceIP & 0xff000000) >> 24,
					(source_trap.ulSourceIP & 0x00ff0000) >> 16,
					(source_trap.ulSourceIP & 0x0000ff00) >> 8,
					(source_trap.ulSourceIP & 0x000000ff)
					);
				for (size_t i = 0; i < source_trap.varArgs.size(); i++)
				{
					utils::remove_str_quotes_and_back_spaces(source_trap.varArgs[i]->wcsVal);
					args.emplace_back(source_trap.varArgs[i]->wcsVal);
				}
				return true;
			}
		private:
		};
	}
}