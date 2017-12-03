#pragma once

#include "NotificationInterface.h"

namespace traphandler
{
	namespace model
	{
		class VoidNotificationHandler : public NotificationInterface
		{
		public:
			bool HostReturnToNormal(Host& host) override
			{
				return true;
			}
			bool NoHeartbeatsFromHost(Host& host) override
			{
				return true;
			}
			bool FileSystemStateChange(
				FileSystem& fs,
				int oldState,
				int newState,
				const std::wstring &thresholdType) override
			{
				return true;
			}
			bool HelloFromHost(Host& host) override
			{
				return true;
			}
			bool GoodbyeFromHost(Host& host) override
			{
				return true;
			}
			bool ProcessStateChange(Host& host) override
			{
				return true;
			}
			bool ServiceStateChange(Host& host) override
			{
				return true;
			}
			bool MaintenanceStart(Host& host) override
			{
				return true;
			}
			bool MaintenanceEnd(Host& host) override
			{
				return true;
			}
		};
	}
}