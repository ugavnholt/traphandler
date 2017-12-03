#pragma once

#include <memory>
#include "Host.h"
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
			FileSystem() {}
			FileSystem(const FileSystem &other)
			{
				*this = other;
			}
			FileSystem& operator=(const FileSystem& other)
			{
				name = other.name;
				id = other.id;
				size = other.size;
				freeMb = other.freeMb;
				status.status = other.status.status;
				hostId = other.hostId;
				pHost = other.pHost;
				thresholdId = other.thresholdId;
				statusChangeTime = other.statusChangeTime;
				return *this;
			}
			std::wstring name;
			int id;
			int64_t size = 0;
			int64_t freeMb = 0;
			Status status;
			int hostId = -1;
			int thresholdId = -1;
			int64_t statusChangeTime = 0;
			Host *pHost = nullptr;
			std::wstring to_wstring()
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
				return str;
			}
			inline double getUtil() const
			{
				double util = (double)(size-freeMb); // used mbs
				util = util / size;	// get ratio
				util = util * 100;	// convert to percent
				return util;
			}
		};
	}
}