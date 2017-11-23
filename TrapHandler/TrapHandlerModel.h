#pragma once

#include <windows.h>
#include <list>
#include "Threshold.h"
#include "FileSystem.h"
#include "Host.h"
#include "ProxyConf.h"
#include "StoreInterface.h"
#include "utils.h"
#include "debugheaders.h"

namespace traphandler
{
	namespace model
	{
		class TrapHandlerModel
		{
		public:
			bool initialize()
			{
				if (!LoadRegInfo())
					return false;

				if (pTrace == nullptr)
				{
					pTrace = CTraceEngine::Initialize(false, 10, 10, 0, 4096);
					bDebug = true;
					pTrace->StartConsoleTrace(LOG_ALL);
				}

				if (!store.isConnected())
				{
					dn(L"MODEL", L"Connecting to database\r\n");
					try
					{
						if (!store.Connect(connectionString))
						{
							dma(L"MODEL", L"Unable to connect to database\r\n");
							return false;
						}
					}
					catch (std::exception& e)
					{
						dma(L"MODEL", L"Exception connecting to database: %S\r\n", e.what());
						return false;
					}
				}
				try
				{
					if (!UpdateConfig(true))
					{
						wprintf(L"Unable to load configuration from database\r\n");
						return false;
					}
				}
				catch (std::exception& e)
				{
					wprintf(L"Exception loading configuration from database: %S\r\n",
						e.what());
					return false;
				}
				return true;
			}

			void SetLogSevFromString(const std::wstring &str)
			{
				if (str == L"ALL")
				{
					pTrace->SetTraceLevel(LOG_ALL, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to ALL\r\n");
					config.minSeverity = L"ALL";
				}
				else if (str == L"DEBUG")
				{
					pTrace->SetTraceLevel(LOG_DEBUG, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to DEBUG\r\n");
					config.minSeverity = L"DEBUG";
				}
				else if (str == L"NORMAL")
				{
					pTrace->SetTraceLevel(LOG_NORMAL, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to NORMAL\r\n");
					config.minSeverity = L"NORMAL";
				}
				else if (str == L"WARNING")
				{
					pTrace->SetTraceLevel(LOG_WARNING, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to WARNING\r\n");
					config.minSeverity = L"WARNING";
				}
				else if (str == L"MINOR")
				{
					pTrace->SetTraceLevel(LOG_MINOR, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to MINOR\r\n");
					config.minSeverity = L"MINOR";
				}
				else if (str == L"MAJOR")
				{
					pTrace->SetTraceLevel(LOG_MAJOR, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to MAJOR\r\n");
					config.minSeverity = L"MAJOR";
				}
				else if (str == L"CRITICAL")
				{
					pTrace->SetTraceLevel(LOG_CRITICAL, LOG_ALL, LOG_ALL, LOG_ALL);
					dd(L"LOG", L"Log level set to CRITICAL\r\n");
					config.minSeverity = L"CRITICAL";
				}
				else
				{
					if (config.minSeverity != L"NORMAL")
					{
						dw(L"LOG", L"The specified log level %s was not understood, defaulting to NORMAL\r\n",
							str.c_str());
						config.minSeverity = L"NORMAL";
						pTrace->SetTraceLevel(LOG_NORMAL, LOG_ALL, LOG_ALL, LOG_ALL);
					}
				}
			}

			bool LoadRegInfo()
			{
				// Read the connection string from registry
				HKEY hKey;
				if (RegOpenKey(HKEY_LOCAL_MACHINE, regRoot.c_str(), &hKey) == ERROR_SUCCESS)
				{

					wchar_t *wcsInstallPath = new wchar_t[MAX_PATH + 1];
					DWORD dwSize = MAX_PATH * sizeof(wchar_t);
					DWORD regType;

					if (RegQueryValueEx(hKey, L"Install Path", 0, &regType, (BYTE*)wcsInstallPath, &dwSize) != ERROR_SUCCESS)
					{
						wprintf(L"unable to read install path\r\n");
						RegCloseKey(hKey);
						return false;
					}

					wchar_t *wcsConnectionString = new wchar_t[65536];
					dwSize = 65535 * sizeof(wchar_t);
					if (RegQueryValueEx(hKey, L"Connection String", 0, &regType, (BYTE*)wcsConnectionString, &dwSize) != ERROR_SUCCESS)
					{
						RegCloseKey(hKey);
						return false;
					}

					dwSize = sizeof(DWORD);
					if (RegQueryValueEx(hKey, L"Grace period seconds", 0, &regType, (BYTE*)&config.gracePeriod, &dwSize) != ERROR_SUCCESS)
					{
						wprintf(L"DWORD Registry key 'Grace period seconds' not defined\r\n");
						RegCloseKey(hKey);
						return false;
					}

					RegCloseKey(hKey);
					connectionString = wcsConnectionString;
					installPath = wcsInstallPath;
					delete[] wcsConnectionString;
					delete[] wcsInstallPath;
				}
				else
					return false;

				return true;
			}
			TrapHandlerModel(StoreInterface& Store,
				const std::wstring &RegRoot)
				: store(Store),
				regRoot(RegRoot)
			{}
			~TrapHandlerModel()
			{
				auto host_it = m_hosts.begin();
				while (host_it != m_hosts.end())
				{
					delete *host_it;
					host_it = m_hosts.erase(host_it);
				}
				auto fs_it = m_filesystems.begin();
				while (fs_it != m_filesystems.end())
				{
					delete *fs_it;
					fs_it = m_filesystems.erase(fs_it);
				}
				auto thresh_it = m_thresholds.begin();
				while (thresh_it != m_thresholds.end())
				{
					delete *thresh_it;
					thresh_it = m_thresholds.erase(thresh_it);
				}
			}
			Threshold &GetThresholdById(uint64_t Id) 
			{
			}
			Threshold &GetThreshold(std::wstring& hostname, std::wstring& fsName)
			{

			}
			FileSystem &GetFilesystem(std::wstring& hostname, std::wstring& fsName)
			{

			}
			FileSystem& GetFilesystemById() 
			{
			}

			void UpdateThresholds(bool initialization = false) 
			{
			}

			void LoadCache(bool initialization = false) 
			{
				std::vector<Host *> loaded_hosts;
				std::vector<FileSystem *> loaded_fs;
				try
				{
					for (int i = 0; i < 100; i++)
					{
						store.LoadHosts(loaded_hosts);
						for (auto it = loaded_hosts.begin(); it != loaded_hosts.end(); it++)
							delete *it;
					}
					auto it = loaded_hosts.begin();
					while (it != loaded_hosts.end())
						delete *it++;
				}
				catch (std::exception &e)
				{
					dma(L"MODEL", L"Exception loading hosts from database: %S\r\n",
						e.what());
				}
				try
				{
					store.LoadFileSystems(loaded_fs);
				}
				catch (std::exception &e)
				{
					dma(L"MODEL", L"Exception loading hosts from database: %S\r\n",
						e.what());

				}
			}
			void UpdateHost(bool initialization = false) 
			{
			}
			void UpdateFileSystem(bool initialization = false) 
			{
			}
			void CreateHost(const Host &NewHost) 
			{
				int newHostId = store.CreateHost(NewHost);
				if (newHostId >= 0)
				{
					Host* newHost = store.LoadHostById(newHostId);
					if (newHost != nullptr)
						m_hosts.push_back(newHost);
					wprintf(L"Loaded host: %s\n", newHost->hostname.c_str());
				}
				
			}
			void CreateFileSystem() {}
			// Reloads the configuration from the database
			// returns true if configuration has changed (except if only minloglevel has
			bool UpdateConfig(bool initialization = false)
			{
				Configuration newConf;
				try
				{
					store.LoadConfiguration(newConf);
				}
				catch (std::exception& e)
				{
					dc(L"CONFIG", L"Exception loading configuration: %S\r\n", e.what());
					return false;
				}
				bool bConfChanged = false;
				if (newConf.hostname != config.hostname)
				{
					bConfChanged = true;
					if(!initialization)
						dd(L"CONFIG", L"Hostname changed from %s to %s\r\n",
							config.hostname.c_str(), newConf.hostname.c_str());
					config.hostname = newConf.hostname;
				}
				if (newConf.snmpQueueLength != config.snmpQueueLength)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"SNMP Queue Length changed from %I64u to %I64u\r\n",
							config.snmpQueueLength, newConf.snmpQueueLength);
					config.snmpQueueLength = newConf.snmpQueueLength;
				}
				if (newConf.cmdQueueLength != config.cmdQueueLength)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Command Queue Length changed from %I64u to %I64u\r\n",
							config.cmdQueueLength, newConf.cmdQueueLength);
					config.cmdQueueLength = newConf.cmdQueueLength;
				}
				if (newConf.heartbeatTimeout != config.heartbeatTimeout)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Heart-beat timeout changed from %I64u to %I64u\r\n",
							config.heartbeatTimeout, newConf.heartbeatTimeout);
					config.heartbeatTimeout = newConf.heartbeatTimeout;
				}
				if (newConf.maxCommands != config.maxCommands)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Max concurrent commands changed from %I64u to %I64u\r\n",
							config.maxCommands, newConf.maxCommands);
					config.maxCommands = newConf.maxCommands;
				}
				if (newConf.updateInterval != config.updateInterval)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Update interval changed from %I64u to %I64u\r\n",
							config.updateInterval, newConf.updateInterval);
					config.updateInterval = newConf.updateInterval;
				}
				if (newConf.ftLastThreshChangeTime > config.ftLastThreshChangeTime)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Thresholds updated\r\n");
					config.ftLastThreshChangeTime = newConf.ftLastThreshChangeTime;
				}
				if (newConf.minSeverity != config.minSeverity)
				{
					SetLogSevFromString(newConf.minSeverity);
				}
				
				return bConfChanged;
			}
			void ProcessTimers() 
			{
				// Loop through all hosts to determine if
				// any of them have been idle for longer than
				// the heart-beat timeout period - and doesn't
				// have a heart-beat failed status
			}

			std::wstring connectionString;
			std::wstring installPath;
			std::wstring regRoot;
			std::vector<Host*> m_hosts;
			std::vector<FileSystem*> m_filesystems;
			std::vector<Threshold*> m_thresholds;
			Configuration config;
		private:
			model::StoreInterface &store;
		}; // class TrapHandlerModel
	} // namespace model
} // namespace traphandler