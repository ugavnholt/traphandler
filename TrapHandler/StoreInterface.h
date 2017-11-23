#pragma once

#include <string>
#include "FileSystem.h"
#include "Host.h"
#include "Threshold.h"
#include "ProxyConf.h"

namespace traphandler
{
	namespace model
	{
		class StoreInterface
		{
		public:
			virtual bool isConnected() const = 0;
			virtual bool Connect(const std::wstring &connStr) = 0;
			virtual void Disconnect() = 0;
			// Loads all hosts from database
			virtual void LoadHosts(std::vector<Host*> &hosts,
				const wchar_t *whereClause = nullptr) = 0;
			virtual Host* LoadHostById(int id) = 0;
			// Loads all filesystems from database
			virtual void LoadFileSystems(std::vector<FileSystem*> &file_systems,
				const wchar_t *whereClause = nullptr) = 0;
			// Loads all thresholds from the database
			virtual void LoadThresholds(std::vector<Threshold*> &thresholds,
				const wchar_t *whereClause = nullptr) = 0;
			// Load config
			virtual void LoadConfiguration(Configuration& config,
				const wchar_t *whereClause = nullptr) = 0;
			// Update filesystem
			virtual void UpdateFileSystem(FileSystem &fs,
				const wchar_t *whereClause = nullptr) = 0;
			// Update HostHelloTime
			virtual void UpdateHostHelloTime(const std::wstring &hostName, uint64_t newHelloTime) = 0;
			// Update HostState
			virtual void UpdateHostState(const std::wstring &hostName, int newState) = 0;
			virtual void CreateFileSystem(const FileSystem &fs) = 0;
			virtual int CreateHost(const Host &host) = 0;
			virtual void CreateThreshold(const Threshold &thresh) = 0;
			Configuration config;
		};
	}
}
