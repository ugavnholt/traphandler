#pragma once

#include <list>
#include "Threshold.h"
#include "FileSystem.h"
#include "Host.h"
#include "ProxyConf.h"
#include "nanodbc.h"

namespace traphandler
{
	namespace model
	{
		class TrapHandlerModel
		{
		public:
			TrapHandlerModel(nanodbc::connection &db_connection)
				: conn(db_connection)
			{}
			Threshold &GetThresholdById() {}
			Threshold &GetThreshold(std::wstring& hostname, std::wstring& fsName)
			{

			}
			FileSystem &GetFilesystem(std::wstring& hostname, std::wstring& fsName)
			{

			}
			FileSystem& GetFilesystemById() {}
			void UpdateThresholds() {}
			void LoadCache() {}
			void UpdateHost() {}
			void UpdateFileSystem() {}
			void CreateHost() {}
			void CreateFileSystem() {}
			void ProcessTimers() 
			{
				// Loop through all hosts to determine if
				// any of them have been idle for longer than
				// the heart-beat timeout period - and doesn't
				// have a heart-beat failed status
			}

			std::vector<Host> m_hosts;
			std::vector<FileSystem> m_filesystems;
			std::vector<Threshold> m_thresholds;
		private:
			nanodbc::connection &conn;
		};
	}
}