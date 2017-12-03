#pragma once

#include "StoreInterface.h"
#include <exception>

#define POCO_STATIC 1

#include <Poco/Data/Session.h>
#include <Poco/Data/ODBC/connector.h>

// using namespace Poco::Data::Keywords;
// using Poco::Data::Session;
// using Poco::Data::Statement;


#define NANODBC_ENABLE_UNICODE 1
#define NANODBC_DISABLE_ASYNC 1

#include "nanodbc.h"

namespace traphandler
{
	namespace model
	{
		class PocoOdbcStore : public StoreInterface
		{
		public:	
			~PocoOdbcStore()
			{
				Disconnect();
			}
			bool isConnected() const override
			{
				return conn.connected() && updateConn.connected();
			}
			bool Connect(const std::wstring &connStr) override
			{
				conn.connect(connStr);
				updateConn.connect(connStr);
				
				selectFilesystemsStatement = new nanodbc::statement(conn);
				selectHostsStatement = new nanodbc::statement(conn);
				createHostStatement = new nanodbc::statement(updateConn);
				selectHostByIdStatement = new nanodbc::statement(conn);
				//selectThresholdsStatement = new nanodbc::statement(conn);
				//helloStatement = new nanodbc::statement(conn);
				//hostStateChangeStatement = new nanodbc::statement(conn);
				//fsMetricStatement = new nanodbc::statement(conn);
				
				//newFsStatement = new nanodbc::statement(conn);

				// prepare our statements
				nanodbc::prepare(*selectHostByIdStatement,
					L" SELECT ID"			// 0
					L",LocalHostName"	// 1
					L",LastSeenIP"		// 2
					L",UGMonVersion"		// 3
					L",VersionString"	// 4
					L",OSVersionMajor"	// 5
					L",OSVersionMinor"	// 6
					L",OSVersionBuild"	// 7
					L",ServicePack"		// 8
					L",Architecture"		// 9
					L",ProxyHostName"	// 10
					L",Platform"			// 11
					L",LastKnownStatus"	// 12
					L", cast(Datediff(s, '1970-01-01',StatusChangeTime) AS bigint)" // 13
					L" FROM Hosts where id=?"
				);
				nanodbc::prepare(*selectHostsStatement,
					L" SELECT ID"			// 0
					L",LocalHostName"	// 1
					L",LastSeenIP"		// 2
					L",UGMonVersion"		// 3
					L",VersionString"	// 4
					L",OSVersionMajor"	// 5
					L",OSVersionMinor"	// 6
					L",OSVersionBuild"	// 7
					L",ServicePack"		// 8
					L",Architecture"		// 9
					L",ProxyHostName"	// 10
					L",Platform"			// 11
					L",LastKnownStatus"	// 12
					L", cast(Datediff(s, '1970-01-01',StatusChangeTime) AS bigint)" // 13
					L" FROM Hosts"
				);
				nanodbc::prepare(*selectFilesystemsStatement,
					L"SELECT [ID]"				// 0
					L", [Name]"					// 1
					L", [LastReportetSizeMB]"	// 2
					L", [LastReportedFreeMB]"	// 3
					L", [LastEvaluatedStatus]"	// 4
					L", [HostID]"				// 5
					L", [LastUsedThresholdID]"	// 6
					L"FROM FileSystems");
				nanodbc::prepare(*createHostStatement,
					L"INSERT INTO [dbo].[Hosts]"
					L"(LocalHostName"
					L",LastSeenIP"
					L",UGMonVersion"
					L",VersionString"
					L",OSVersionMajor"
					L",OSVersionMinor"
					L",OSVersionBuild"
					L",ServicePack"
					L",Architecture"
					L",ProxyHostName"
					L",Platform"
					L",LastKnownStatus)"
					L" VALUES"
					L"(?" // hostname
					L",?" // lastSeenIp
					L",?" // ugmon version
					L",?" // version string
					L",?" // os major version
					L",?" // os minor version
					L",?" // build number
					L",?" // service pack
					L",?" // architecture
					L",?" // proxy host name
					L",?" // platform
					L",0)"// LastKnownStatus
					);
				
				//nanodbc::prepare(helloStatement, L"insert into public.simple_test (a, b) values (?, ?);");
				//nanodbc::prepare(hostStateChangeStatement, L"insert into public.simple_test (a, b) values (?, ?);");
				//nanodbc::prepare(fsMetricStatement, L"insert into public.simple_test (a, b) values (?, ?);");
				//nanodbc::prepare(newHostStatement, L"insert into public.simple_test (a, b) values (?, ?);");
				//nanodbc::prepare(newFsStatement, L"insert into public.simple_test (a, b) values (?, ?);");
				return true;
			}
			void Disconnect() override
			{
				try
				{
					conn.disconnect();
					updateConn.disconnect();
					if (selectFilesystemsStatement != nullptr)
					{
						delete selectFilesystemsStatement;
						selectFilesystemsStatement = nullptr;
					}
					if (selectHostsStatement != nullptr)
					{
						delete selectHostsStatement;
						selectHostsStatement = nullptr;
					}
					if (selectHostByIdStatement != nullptr)
					{
						delete selectHostByIdStatement;
						selectHostByIdStatement = nullptr;
					}
					if (selectThresholdsStatement != nullptr)
					{
						delete selectThresholdsStatement;
						selectThresholdsStatement = nullptr;
					}
					if (helloStatement != nullptr)
					{
						delete helloStatement;
						helloStatement = nullptr;
					}
					if (hostStateChangeStatement != nullptr)
					{
						delete hostStateChangeStatement;
						hostStateChangeStatement = nullptr;
					}
					if (fsMetricStatement != nullptr)
					{
						delete fsMetricStatement;
						fsMetricStatement = nullptr;
					}
					if (createHostStatement != nullptr)
					{
						delete createHostStatement;
						createHostStatement = nullptr;
					}
					if (newFsStatement != nullptr)
					{
						delete newFsStatement;
						newFsStatement = nullptr;
					}
				}
				catch (const std::exception&)
				{
				}
			}
			// Loads hosts from database
			void LoadHosts(std::vector<Host*> &hosts, 
				const wchar_t *whereClause = nullptr) override
			{
				try
				{
					auto results = selectHostsStatement->execute();
					
					while (results.next())
					{
						Host* newHost = CreateHostFromDbRow(results);
						hosts.push_back(newHost);
					}
				}
				catch (...)
				{
					// cleanup partial host array
					auto it = hosts.begin();
					while (it != hosts.end())
					{
						delete *it;
						it = hosts.erase(it);
					}
					
					throw;
				}
			}

			Host* LoadHostById(int id) override
			{
				selectHostByIdStatement->bind(0, &id);
				try
				{
					auto results = selectHostByIdStatement->execute();
					if (results.next())
						return CreateHostFromDbRow(results);
					else
						return nullptr;
				}
				catch (...)
				{
					throw;
				}
			}
			// Loads all filesystems from database
			void LoadFileSystems(std::vector<FileSystem*> &file_systems,
				const wchar_t *whereClause = nullptr) override
			{
				try
				{
					auto results = selectFilesystemsStatement->execute();

					while (results.next())
					{
						auto newFs = new FileSystem;
						newFs->id = results.get<int>(0);
						newFs->name = results.get<std::wstring>(1);
						newFs->size = results.get<int64_t>(2);
						newFs->freeMb = results.get<int64_t>(3);
						newFs->status.setStatus(results.get<int>(4));
						newFs->hostId = results.get<int>(5);
						if (!results.is_null(6))
							newFs->thresholdId = results.get<int>(6);
						else
							newFs->thresholdId = -1;
						file_systems.push_back(newFs);
					}
				}
				catch (...)
				{
					// cleanup partial host array
					auto it = file_systems.begin();
					while (it != file_systems.end())
					{
						delete *it;
						it = file_systems.erase(it);
					}

					throw;
				}
			}
			// Loads all thresholds from the database
			void LoadThresholds(std::vector<Threshold*> &thresholds,
				const wchar_t *whereClause = nullptr) override
			{

				
			}
			
			// Load config
			void LoadConfiguration(Configuration& config, 
				const wchar_t *whereClause = nullptr) override
			{
				std::wstring query(
					L"SELECT [ProxyHostName]"		// 0
					L", [SNMPQueueLength]"			// 1
					L", [CommandQueueLength]"		// 2
					L", [HeartBeatTimeoutSecs]"		// 3
					L", [MaxConcurrentCommands]"	// 4
					L", [ConfigUpdateIntervalSecs]"	// 5
					L", upper(LogMinSeverity)"			// 6
					L", cast(Datediff(s, '1970-01-01',ThreshChangeTime) AS bigint)" // 7
					L"FROM Proxies");
				if (whereClause != nullptr)
				{
					query += L" WHERE ";
					query += whereClause;
				}
				auto results = nanodbc::execute(conn,
					query);
				results.next();
				config.hostname = results.get <nanodbc::string>(0);
				config.snmpQueueLength = results.get<int64_t>(1);
				config.cmdQueueLength = results.get<int64_t>(2);
				config.heartbeatTimeout = results.get<int64_t>(3);
				config.maxCommands = results.get<int64_t>(4);
				config.updateInterval = results.get<int64_t>(5);
				config.minSeverity = results.get<std::wstring>(6);
				config.ftLastThreshChangeTime = results.get<uint64_t>(7);

				config.ftLastThreshChangeTime = utils::unixtime2filetime(config.ftLastThreshChangeTime);
			}
			// Update filesystem
			void UpdateFileSystem(FileSystem &fs,
				const wchar_t *whereClause = nullptr) override
			{
			}
			// Update HostHelloTime
			void UpdateHostHelloTime(const std::wstring &hostName, uint64_t newHelloTime) override
			{
			}
			// Update HostState
			void UpdateHostState(const std::wstring &hostName, int newState) override
			{
			}
			void CreateFileSystem(const FileSystem &fs) override
			{}
			int CreateHost(const Host &host) override 
			{
				createHostStatement->bind(0, host.hostname.c_str());
				createHostStatement->bind(1, host.ip.c_str());
				createHostStatement->bind(2, host.ugmonVersion.c_str());
				createHostStatement->bind(3, host.versionString.c_str());
				createHostStatement->bind(4, &host.osVersionMajor);
				createHostStatement->bind(5, &host.osVersionMinor);
				createHostStatement->bind(6, host.osVersionBuild.c_str());
				createHostStatement->bind(7, &host.servicePack);
				createHostStatement->bind(8, host.architecture.c_str());
				createHostStatement->bind(9, host.proxyHostName.c_str());
				createHostStatement->bind(10, host.platform.c_str());
				int newId = -1;
				try
				{
					auto results = createHostStatement->execute();
					//while(results.next())
					//	newId = results.get<int>(0);
					newId = -1;
				}
				catch (...)
				{
					throw;
				}
				return newId;
			}
			void CreateThreshold(const Threshold &thresh) override
			{}
		private:
			std::wstring connectionString;
			nanodbc::connection conn;
			nanodbc::connection updateConn;
			nanodbc::statement* selectFilesystemsStatement = nullptr;
			nanodbc::statement* selectHostsStatement = nullptr;
			nanodbc::statement* selectHostByIdStatement = nullptr;
			nanodbc::statement* selectThresholdsStatement = nullptr;
			nanodbc::statement* helloStatement = nullptr;
			nanodbc::statement* hostStateChangeStatement = nullptr;
			nanodbc::statement* fsMetricStatement = nullptr;
			nanodbc::statement* createHostStatement = nullptr;
			nanodbc::statement* newFsStatement = nullptr;

			Host* CreateHostFromDbRow(nanodbc::result &row)
			{
				Host* newHost = new Host;
				try
				{
					newHost->id = row.get<int>(0);
					newHost->hostname = row.get<std::wstring>(1);
					newHost->ip = row.get<std::wstring>(2);
					newHost->ugmonVersion = row.get<std::wstring>(3);
					if (!row.is_null(4))
						newHost->versionString = row.get<std::wstring>(4);
					if (!row.is_null(5))
						newHost->osVersionMajor = row.get<int>(5);
					if (!row.is_null(6))
						newHost->osVersionMinor = row.get<int>(6);
					if (!row.is_null(7))
						newHost->osVersionBuild = row.get<std::wstring>(7);
					if (!row.is_null(8))
						newHost->servicePack = row.get<int>(8);
					if (!row.is_null(9))
						newHost->architecture = row.get<std::wstring>(9);
					newHost->proxyHostName = row.get<std::wstring>(10);
					newHost->platform = row.get<std::wstring>(11);
					newHost->status.setStatus(row.get<int>(12));
					newHost->statusChangeTime = row.get<uint64_t>(13);
				}
				catch (...)
				{
					delete newHost;
					newHost = nullptr;
					throw;
				}
				return newHost;
			}
		};

	}
}