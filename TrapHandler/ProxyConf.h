#pragma once

namespace traphandler
{
	namespace model
	{
		class Configuration
		{
		public:
			std::wstring	hostname;
			uint64_t		snmpQueueLength = 0;
			uint64_t		cmdQueueLength = 0;
			uint64_t		heartbeatTimeout = 0;
			uint64_t		maxCommands = 0;
			uint64_t		updateInterval = 0;
			uint64_t		gracePeriod = 0;
			std::wstring	minSeverity;
			uint64_t		ftLastThreshChangeTime = 0;
			std::wstring to_wstring() const
			{
				std::wstring str;
				return str;
			}
		};
	}
}