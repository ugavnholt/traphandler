#pragma once

#include "ExecObj.h"
#include "NotificationInterface.h"
#include "CmdQueueInterface.h"
#include "utils.h"
#include <algorithm>

namespace traphandler
{
	namespace model
	{
		class OpcNotificationHandler : public NotificationInterface
		{
		public:
			OpcNotificationHandler(traphandler::CmdQueueInterface &queueHandler)
				: cmdQueue(queueHandler)
			{}

			bool HostReturnToNormal(Host& host, const std::wstring &timerId) override
			{
				std::wstring cmd(L"opcmsg a=UGMon o=aliveMessage sev=Normal -node=");
				cmd += host.hostname;
				cmd += L" msg_t=\"host='";
				cmd += host.hostname;
				cmd += L"' platform='";
				cmd += host.platform;
				cmd += L"' timer_id='";
				cmd += timerId;
				cmd += L"'\"";
				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool NoHeartbeatsFromHost(Host& host) override
			{
				return true;
			}
			bool FileSystemStateChange(
				const Host &host,
				const FileSystem& fs, 
				const Threshold& thresh,
				int oldState, 
				int newState, 
				const std::wstring &thresholdType) override
			{
				std::wstring cmd(L"opcmsg a=UGMon o=Threshold -node=");
				cmd += host.hostname;
				cmd += L" msg_t=\"Hostname='";
				cmd += host.hostname;
				cmd += L"' threshType='";
				cmd += thresholdType;
				cmd += L"' platform='";
				cmd += host.platform;
				switch (newState)
				{
				case 2:
					cmd += L"' newState='HIGH'";
					break;
				case 1:
					cmd += L"' newState='WARN'";
					break;
				default:
					cmd += L"' newState='NORM'";
					break;
				}
				switch (oldState)
				{
				case 2:
					cmd += L" oldState='HIGH' volume='";
					break;
				case 1:
					cmd += L" oldState='WARN' volume='";
					break;
				default:
					cmd += L" oldState='NORM' volume='";
					break;
				}
				cmd += fs.name;
				cmd += L"' ";
				wchar_t tmpStr2[250];
				swprintf(tmpStr2, 249, 
					L"FreeMB='%I64u' totMB='%I64u' percent='%.2f' ThreshMBWarn='%I64u' ThreshMBHigh='%I64u' ThreshUtilWarn='%.2f' ThreshUtilHigh='%.2f'",
					fs.freeMb, 
					fs.size, 
					fs.getUtil(), 
					thresh.WarnFreeMegsThresh, 
					thresh.HighFreeMegsThresh,
					thresh.WarnUtilThresh, 
					thresh.HighUtilThresh);
				cmd += tmpStr2;
				cmd += L"\" sev=";

				if (newState == 0)
					cmd += L"Normal";
				else if (newState == 1)
					cmd += thresh.WarnSevStr;
				else
					cmd += thresh.HighSevStr;
				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool HelloFromHost(Host& host) override
			{
				std::wstring cmd(L"opcmsg a=UGMon o=helloMessage -node=");
				cmd += host.hostname;
				cmd += L" msg_t=\"host='";
				cmd += host.hostname;
				cmd += L"' platform='";
				cmd += host.platform;
				cmd += L"' OSFlavor='";
				cmd += host.architecture;
				cmd += L"' SourceIP='";
				cmd += host.ip;
				cmd += L"' UGMonVer='";
				cmd += host.ugmonVersion;
				cmd += L"'\"";
				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool GoodbyeFromHost(Host& host) override
			{
				std::wstring cmd(L"opcmsg a=UGMon o=goodbyeMessage -node=");
				cmd += host.hostname;
				cmd += L" msg_t=\"host='";
				cmd += host.hostname;
				cmd += L"' platform='";
				cmd += host.platform;
				cmd += L"' UGMonVer='";
				cmd += host.ugmonVersion;
				cmd += L"'\"";
				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool ProcessStateChange(Host& host, 
				const std::wstring &messageGroup,
				const std::wstring &defaultMessageGroup,
				const std::wstring &severity,
				const std::wstring &processName,
				const std::wstring &currentState,
				const std::wstring &desiredState,
				bool post1_13) override
			{
				std::wstring cmd;
				if (post1_13)
					cmd = L"opcmsg a=UGMon1_13 o=processState -node=";
				else
					cmd = L"opcmsg a=UGMon o=processState -node=";
				cmd += host.hostname;
				cmd += L" msg_grp=";
				cmd += messageGroup;
				if (utils::iequals(messageGroup, defaultMessageGroup))
					cmd += host.platform;
				cmd += L" sev=";
				cmd += severity;
				cmd += L" msg_t=\"host='";
				cmd += host.hostname;
				cmd += L"' process='";
				cmd += processName;
				cmd += L"' currentState='";
				cmd += currentState;
				cmd += L"' desiredState='";
				cmd += desiredState;
				cmd += L"' platform='";
				cmd += host.platform;
				cmd += L"'\" -option currentState=";
				cmd += currentState;
				cmd += L" -option desiredState=";
				cmd += desiredState;
				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool ServiceStateChange(Host& host,
				const std::wstring &messageGroup,
				const std::wstring &defaultMessageGroup,
				const std::wstring &severity,
				const std::wstring &serviceName,
				const std::wstring &currentState,
				const std::wstring &desiredState,
				std::wstring &serviceCaption,
				bool post1_13) override
			{
				std::wstring cmd;
				if (post1_13)
					cmd = L"opcmsg a=UGMon1_13 o=serviceState -node=";
				else
					cmd = L"opcmsg a=UGMon o=serviceState -node=";
				cmd += host.hostname;
				cmd += L" msg_grp=";
				cmd += messageGroup;
				if(utils::iequals(messageGroup, defaultMessageGroup))
					cmd += host.platform;
				cmd += L" sev=";
				cmd += severity;
				cmd += L" msg_t=\"host='";
				cmd += host.hostname;
				cmd += L"' service='";
				cmd += serviceName;
				cmd += L"' currentState='";
				cmd += currentState;
				cmd += L"' desiredState='";
				cmd += desiredState;
				cmd += L"' platform='";
				cmd += host.platform;
				cmd += L"'\" -option currentState=";
				cmd += currentState;
				cmd += L" -option desiredState=";
				cmd += desiredState;
				cmd += L" -option label=\"";
				std::replace(serviceCaption.begin(), serviceCaption.end(), L'\"', L'\'');
				cmd += serviceCaption;
				cmd += L"\"";

				if (cmdQueue.QueueCommand(cmd))
					return true;
				return false;
			}
			bool MaintenanceStart(Host& host) override
			{
				return true;
			}
			bool MaintenanceEnd(Host& host) override
			{
				return true;
			}

		private:
			traphandler::CmdQueueInterface& cmdQueue;
		};
	}
}
