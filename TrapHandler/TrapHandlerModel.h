#pragma once

#include <windows.h>
#include <list>
#include <map>
#include "Threshold.h"
#include "FileSystem.h"
#include "Host.h"
#include "ProxyConf.h"
#include "StoreInterface.h"
#include "utils.h"
#include "debugheaders.h"
#include <tuple>
#include "NotificationInterface.h"

namespace traphandler
{
	namespace model
	{
		typedef std::tuple<int, int> fsId;
		typedef std::tuple<std::wstring, std::wstring> fsNameId;

		struct fsIdComparer
		{
		public:
			bool operator()(fsId &x, fsId& y)
			{
				if (std::get<0>(x) < std::get<0>(y))
					return true;
				if (std::get<1>(x) < std::get<1>(y))
					return true;
				return false;
			}
		};
		struct fsNameComparer
		{
		public:
			bool operator()(fsId &x, fsId& y)
			{
				if (std::get<0>(x) < std::get<0>(y))
					return true;
				if (std::get<1>(x) < std::get<1>(y))
					return true;
				return false;
			}
		};
		

		typedef std::map<int, Host*> HostIdMap;
		typedef std::map<std::wstring, Host*> HostNameMap;
		typedef std::map<std::wstring, FileSystem*,fsNameComparer> FsNameMap;
		typedef std::map<int, FileSystem*, fsIdComparer> FsIdMap;


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
				NotificationInterface& NotificationHandler,
				const std::wstring &RegRoot)
				: store(Store),
				regRoot(RegRoot),
				notificationHandler(NotificationHandler)
			{}
			~TrapHandlerModel()
			{
				ReleaseAll();
			}

			// Action processers for received messages

			bool HelloFromHost()
			{
				//action.HelloFromHost();
			}

			bool GoodbyeFromHost()
			{
				//action.GoodbyeFromHost();
			}

			bool FSMetricFromHost()
			{
				
			}

			bool MaintenanceEventFromHost()
			{
				//action.MaintenanceStart();
			}

			bool ProcessStateEventFromHost()
			{}

			bool ServiceStateEventFromHost()
			{}

			bool AgentEvent()
			{}

			bool ClusterResourceStateEventFromHost()
			{}

			bool ErrorEventFromHost()
			{}


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
				LoadThresholds();
				LoadHosts();
				LoadFileSystems();
			}
			void UpdateHost(bool initialization = false) 
			{
			}
			void UpdateFileSystem(bool initialization = false) 
			{
			}
			void CreateHost(Host &NewHost) 
			{
				try
				{
					store.CreateHost(NewHost);
				}
				catch(std::exception &ex)
				{ 
					dma(L"MODEL", L"Exception creating host: %S\r\n",
						ex.what());
					throw;
				}
			}
			void CreateFileSystem(FileSystem& newFs) 
			{
				try
				{
					store.CreateFileSystem(newFs);
				}
				catch (std::exception &ex)
				{
					dma(L"MODEL", L"Exception creating file system: %S\r\n",
						ex.what());
					throw;
				}
			}
			void DeleteAllObjects()
			{
				try
				{
					store.DeleteAllObjects();
					ReleaseAll();
				}
				catch (std::exception &ex)
				{
					dma(L"MODEL", L"Exception deleting all objects: %S\r\n",
						ex.what());
					throw;
				}
			}
			// Reloads the configuration from the database
			// returns true if configuration has changed (except if only minloglevel has
			bool UpdateConfig(bool initialization = false)
			{
				try
				{
					store.LoadConfiguration();
				}
				catch (std::exception& e)
				{
					dc(L"CONFIG", L"Exception loading configuration: %S\r\n", e.what());
					return false;
				}
				if (store.latestConfig.hostname.empty())
				{
					dc(L"CONFIG", L"The loaded configuration seems incomplete, no proxyhostname defined\r\n");
					return false;
				}
				bool bConfChanged = false;
				if (store.latestConfig.hostname != config.hostname)
				{
					bConfChanged = true;
					if(!initialization)
						dd(L"CONFIG", L"Hostname changed from %s to %s\r\n",
							config.hostname.c_str(), store.latestConfig.hostname.c_str());
					config.hostname = store.latestConfig.hostname;
				}
				if (store.latestConfig.snmpQueueLength != config.snmpQueueLength)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"SNMP Queue Length changed from %I64u to %I64u\r\n",
							config.snmpQueueLength, store.latestConfig.snmpQueueLength);
					config.snmpQueueLength = store.latestConfig.snmpQueueLength;
				}
				if (store.latestConfig.cmdQueueLength != config.cmdQueueLength)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Command Queue Length changed from %I64u to %I64u\r\n",
							config.cmdQueueLength, store.latestConfig.cmdQueueLength);
					config.cmdQueueLength = store.latestConfig.cmdQueueLength;
				}
				if (store.latestConfig.heartbeatTimeout != config.heartbeatTimeout)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Heart-beat timeout changed from %I64u to %I64u\r\n",
							config.heartbeatTimeout, store.latestConfig.heartbeatTimeout);
					config.heartbeatTimeout = store.latestConfig.heartbeatTimeout;
				}
				if (store.latestConfig.maxCommands != config.maxCommands)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Max concurrent commands changed from %I64u to %I64u\r\n",
							config.maxCommands, store.latestConfig.maxCommands);
					config.maxCommands = store.latestConfig.maxCommands;
				}
				if (store.latestConfig.updateInterval != config.updateInterval)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Update interval changed from %I64u to %I64u\r\n",
							config.updateInterval, store.latestConfig.updateInterval);
					config.updateInterval = store.latestConfig.updateInterval;
				}
				if (store.latestConfig.ftLastThreshChangeTime > config.ftLastThreshChangeTime)
				{
					bConfChanged = true;
					if (!initialization)
						dd(L"CONFIG", L"Thresholds updated\r\n");
					config.ftLastThreshChangeTime = store.latestConfig.ftLastThreshChangeTime;
				}
				if (store.latestConfig.minSeverity != config.minSeverity)
				{
					SetLogSevFromString(store.latestConfig.minSeverity);
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

			Host* GetHostById(int hostId)
			{
				Host* foundHost = nullptr;
				auto found = m_hosts.find(hostId);
				if (found != m_hosts.end())
					foundHost = found->second;
				return foundHost;
			}
			Host* GetHostByName(std::wstring &hostName)
			{
				Host* foundHost = nullptr;
				auto found = m_hostsByName.find(hostName);
				if (found != m_hostsByName.end())
					foundHost = found->second;
				return foundHost;
			}
			FileSystem *GetFsByHostIdAndName(int hostId, std::wstring fsName)
			{

			}

			std::wstring connectionString;
			std::wstring installPath;
			std::wstring regRoot;
			HostIdMap m_hosts;
			HostNameMap m_hostsByName;
			FsIdMap m_filesystems;
			FsNameMap m_filesystemsByName;
			std::vector<Threshold*> m_thresholds;
			Configuration config;
		private:
			model::StoreInterface &store;
			model::NotificationInterface &notificationHandler;
			void LoadThresholds()
			{
				std::vector<Threshold*> loaded_thresholds;
				try
				{
					store.LoadThresholds(loaded_thresholds);
					dn(L"MODEL", L"Loaded %zi thresholds from database\r\n",
						loaded_thresholds.size());
					// check for any thresholds that are in source, and not in dest
					// and thresholds that are in dest, but not in source
					m_thresholds.clear;
					m_thresholds = loaded_thresholds;
				}
				catch (std::exception &e)
				{
					dma(L"MODEL", L"Exception loading thresholds from database: %S\r\n",
						e.what());
					auto thresh_it = loaded_thresholds.begin();
					while (thresh_it != loaded_thresholds.end())
					{
						delete *thresh_it;
						thresh_it = loaded_thresholds.erase(thresh_it);
					}
				}
			}

			void LoadHosts()
			{
				std::vector<Host *> loaded_hosts;
				try
				{
					store.LoadHosts(loaded_hosts);
					dn(L"MODEL", L"Loaded %zi host entries from database\r\n",
						loaded_hosts.size());

					m_hostsByName.clear();

					for (auto it = loaded_hosts.begin(); it != loaded_hosts.end(); it++)
					{
						// if a host exists with the same id - get it - remove its entry from the
						// hostnames dict, delete old object, and insert the pointer to the two
						// host lookup tables
						bool exists = m_hostsByName.find((*it)->hostname) == m_hostsByName.end();
						if (exists)
						{
							dma(L"MODEL", L"Duplicate host - a host with hostname %s already exists\r\n",
								(*it)->hostname.c_str());
							continue;
						}
						auto existing = m_hosts.find((*it)->id);
						if (existing != m_hosts.end())
						{
							// id already exists
							delete existing->second;
							existing->second = (*it);
						}
						else
						{
							// new host, we havent seen before
							m_hosts.insert(std::pair<int, Host*>((*it)->id, (*it)));

						}
						m_hostsByName.insert(std::pair<std::wstring, Host*>((*it)->hostname, (*it)));
					}
				}
				catch (std::exception &e)
				{
					dma(L"MODEL", L"Exception loading hosts from database: %S\r\n",
						e.what());
					auto host_it = loaded_hosts.begin();
					while (host_it != loaded_hosts.end())
					{
						delete *host_it;
						host_it = loaded_hosts.erase(host_it);
					}
				}
			}

			void LoadFileSystems()
			{
				std::vector<FileSystem *> loaded_fs;
				try
				{
					store.LoadFileSystems(loaded_fs);
					dn(L"MODEL", L"Loaded %zi file system entries from database\r\n",
						loaded_fs.size());
					// for each file system, load the related host
					for (auto it = loaded_fs.begin(); it != loaded_fs.end(); it++)
					{
						if ((*it)->hostId >= 0)
						{
							auto foundHost = m_hosts.find((*it)->hostId);
							if (foundHost == m_hosts.end())
							{
								dmi(L"MODEL", L"Unable to find host with id: %i, for file system: %s\r\n",
									(*it)->hostId,
									(*it)->to_wstring());
								continue;
							}
							// set the file system pointer
							(*it)->pHost = foundHost->second;
							(*it)->hostId = foundHost->second->id;
						}
					}
				}
				catch (std::exception &e)
				{
					dma(L"MODEL", L"Exception loading file systems from database: %S\r\n",
						e.what());
					auto fs_it = loaded_fs.begin();
					while (fs_it != loaded_fs.end())
					{
						delete *fs_it;
						fs_it = loaded_fs.erase(fs_it);
					}
				}
			}

			void ReleaseAll()
			{
				auto host_it = m_hosts.begin();
				while (host_it != m_hosts.end())
				{
					delete host_it->second;
					host_it = m_hosts.erase(host_it);
				}
				auto fs_it = m_filesystems.begin();
				while (fs_it != m_filesystems.end())
				{
					delete fs_it->second;
					fs_it = m_filesystems.erase(fs_it);
				}
				auto thresh_it = m_thresholds.begin();
				while (thresh_it != m_thresholds.end())
				{
					delete *thresh_it;
					thresh_it = m_thresholds.erase(thresh_it);
				}
			}
		}; // class TrapHandlerModel
	} // namespace model
} // namespace traphandler