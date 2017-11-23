#pragma once

#include <memory>
#include "Status.h"

namespace traphandler
{
	namespace model
	{
		class FileSystem;
		typedef std::shared_ptr<FileSystem> pFileSystem;
		class FileSystem
		{
		public:
			std::wstring name;
			int id;
			int64_t size;
			int64_t freeMb;
			int64_t util;
			Status status;
			int hostId;
			int thresholdId;
			std::wstring ToString()
			{
				std::wstring str = L"FileSystem (";
				str += std::to_wstring(id);
				str += L") ";
				str += name;
				str += L" size: ";
				str += std::to_wstring(size);
				str += L"mb - free space: ";
				str += std::to_wstring(freeMb);
				str += L" last status: ";
				std::to_wstring(status.getStatus());
				str += name;
			}
		};
	}
}