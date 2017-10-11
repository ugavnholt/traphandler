#pragma once

namespace traphandler
{
	namespace model
	{

		class FileSystem
		{
		public:
			std::wstring name;
			int64_t id;
			int64_t size;
			int64_t freeMb;
			int64_t util;
			int status;
			uuid_t hostId;
			int64_t thresholdId;
		};
	}
}