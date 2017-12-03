#pragma once

#include "Host.h"
#include "FileSystem.h"
#include "Threshold.h"

namespace traphandler
{
	namespace model
	{
		class NotificationInterface
		{
		public:
			virtual bool HostReturnToNormal(Host& host, const std::wstring &timerId) = 0;
			virtual bool NoHeartbeatsFromHost(Host& host) = 0;
			virtual bool FileSystemStateChange(
				const Host &host,
				const FileSystem& fs,
				const Threshold& thresh,
				int oldState, 
				int newState, 
				const std::wstring &thresholdType) = 0;
			virtual bool HelloFromHost(Host& host) = 0;
			virtual bool GoodbyeFromHost(Host& host) = 0;
			virtual bool ProcessStateChange(Host& host, 
				const std::wstring &msgGrp, 
				const std::wstring &defaultMessageGroup,
				const std::wstring &severity,
				const std::wstring &processName,
				const std::wstring &currentState,
				const std::wstring &desiredState,
				bool post1_13) = 0;
			virtual bool ServiceStateChange(
				Host& host,
				const std::wstring &msgGrp,
				const std::wstring &defaultMessageGroup,
				const std::wstring &severity,
				const std::wstring &serviceName,
				const std::wstring &currentState,
				const std::wstring &desiredState,
				std::wstring &serviceCaption,
				bool post1_13) = 0;
			virtual bool MaintenanceStart(Host& host) = 0;
			virtual bool MaintenanceEnd(Host& host) = 0;
		};
	}
}
