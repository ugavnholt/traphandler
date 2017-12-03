#pragma once

#include "StoreInterface.h"
#include <exception>
#include "utils.h"

#define POCO_STATIC 1

#include <Poco/Data/Session.h>
#include <Poco/Data/ODBC/connector.h>
#include <Poco/Data/RecordSet.h>

#pragma region Queries
const char* const selectHostAttributes =
	"SELECT ID"			// 0
	",LocalHostName"	// 1
	",LastSeenIP"		// 2
	",UGMonVersion"		// 3
	",VersionString"	// 4
	",OSVersionMajor"	// 5
	",OSVersionMinor"	// 6
	",OSVersionBuild"	// 7
	",ServicePack"		// 8
	",Architecture"		// 9
	",Platform"			// 10
	",LastKnownStatus"	// 11
	",cast(Datediff(s, '1970-01-01',StatusChangeTime) AS bigint)" // 12
	" FROM Hosts";

const char* const selectThresholdAttributes =
	"SELECT ID"			// 0
	",HostExpression"	// 1
	",VolumeExpression"	// 2
	",TreshFreeMBWarn"	// 3
	",ThreshFreeMBHigh"	// 4
	",ThreshUtilWarn"	// 5
	",ThreshUtilHigh"	// 6
	",WarnSev"			// 7
	",highSev"			// 8
	",MinFSSizeMb"		// 9
	",MaxFSSizeMb"		// 10
	" FROM FSThresholds";
const char* const selectFileSystemAttributes =
	"select ID"				// 0
	",name"					// 1
	",LastReportetSizeMB"	// 2
	",LastReportedFreeMB"	// 3
	",LastEvaluatedStatus"	// 4
	",LastUsedThresholdID"	// 5
	",HostId"				// 6
	",cast(Datediff(s, '1970-01-01',StatusChangeTime) AS bigint)" // 7
	" FROM FileSystems";
const char* const createHostSql = 
	"INSERT INTO Hosts"
	"(LocalHostName"
	",LastSeenIP"
	",UGMonVersion"
	",VersionString"
	",OSVersionMajor"
	",OSVersionMinor"
	",OSVersionBuild"
	",ServicePack"
	",Architecture"
	",ProxyHostName"
	",Platform)"
	" OUTPUT INSERTED.id"
	",cast(Datediff(s, '1970-01-01',INSERTED.StatusChangeTime) AS bigint)"
	" VALUES"
	"(?" // hostname			// 0
	",?" // lastSeenIp			// 1
	",?" // ugmon version		// 2
	",?" // version string		// 3
	",?" // os major version	// 4
	",?" // os minor version	// 5
	",?" // build number		// 6
	",?" // service pack		// 7
	",?" // architecture		// 8
	",?" // proxy host name		// 9
	",?)" // platform			// 10
	;
const char* const createFsSql =
	"INSERT INTO FileSystems"
	"(Name"
	",LastReportedSizeMB"
	",lastReportedFreeMB"
	",HostId) OUTPUT INSERTED.id"
	",cast(Datediff(s, '1970-01-01',INSERTED.StatusChangeTime) AS bigint)"
	" VALUES "
	"(?,?,?,?)"
	;
const char* const createThresholdSql =
"INSERT INTO FSThresholds"
"([EvalOrder]"
", [HostExpression]"
", [VolumeExpression]"
", [TreshFreeMBWarn]"
", [ThreshFreeMBHigh]"
", [ThreshUtilWarn]"
", [ThreshUtilHigh]"
", [WarnSev]"
", [highSev]"
", [MinFSSizeMb]"
", [MaxFSSizeMb]"
", [isLocked])"
"VALUES"
"(?,?,?,?,?,?,?,?,?,?,?,?)";
/*"(<EvalOrder, int, >"
", <HostExpression, nvarchar(max), >"
", <VolumeExpression, nvarchar(max), >"
", <TreshFreeMBWarn, int, >"
", <ThreshFreeMBHigh, int, >"
", <ThreshUtilWarn, float, >"
", <ThreshUtilHigh, float, >"
", <WarnSev, nvarchar(20), >"
", <highSev, nvarchar(20), >"
", <MinFSSizeMb, int, >"
", <MaxFSSizeMb, int, >"
", <isLocked, int, >)"
""
;*/

#pragma endregion Queries

namespace traphandler
{
	namespace model 
	{

		class PocoOdbcStore : public StoreInterface
		{
		public:	
			PocoOdbcStore()
			{
				Poco::Data::ODBC::Connector::registerConnector();
			}
			~PocoOdbcStore()
			{
				Disconnect();
				Poco::Data::ODBC::Connector::unregisterConnector();
			}
			bool isConnected() const override
			{
				if (session == nullptr)
					return false;
				return session->isConnected();
			}
			bool Connect(const std::wstring &connStr) override
			{
				// initialize a ascoo version of the connection string
				connectionString = utils::ws2s(connStr);
				try
				{
					if (session == nullptr)
						session = new Poco::Data::Session("ODBC", connectionString, 20);

				}
				catch (Poco::Data::ConnectionFailedException &ex)
				{
					dc(L"ODBC", L"Database connection failed: %S\r\n", ex.what());
					throw;
				}
				return true;
			}
			void Disconnect() override
			{
				if(session != nullptr)
				{
					try
					{
						if (selectConfigStmt != nullptr)
						{
							delete selectConfigStmt;
							selectConfigStmt = nullptr;
						}
						if (selectAllHostsStmt != nullptr)
						{
							delete selectAllHostsStmt;
							selectAllHostsStmt = nullptr;
						}
						if (selectAllThresholdsStmt != nullptr)
						{
							delete selectAllThresholdsStmt;
							selectAllThresholdsStmt = nullptr;
						}
						if (selectAllFileSystemsStmt != nullptr)
						{
							delete selectAllFileSystemsStmt;
							selectAllFileSystemsStmt = nullptr;
						}
						if (createHostStmt != nullptr)
						{
							delete createHostStmt;
							createHostStmt = nullptr;
						}
						if (createFsStmt != nullptr)
						{
							delete createFsStmt;
							createFsStmt = nullptr;
						}
						if (session->isConnected())
							session->close();
						delete session;
						session = nullptr;
					}
					catch (const std::exception& e)
					{
						dma(L"STORE", L"Exception destroying store connection, %S\r\n",
							e.what());
					}
				}
				
			}
			// Loads hosts from database
			void LoadHosts(std::vector<Host*> &hosts) override
			{
				hosts.clear();
				if (selectAllHostsStmt == nullptr)
				{
					selectAllHostsStmt = new Poco::Data::Statement(*session);
					*selectAllHostsStmt << selectHostAttributes;
				}
				try
				{
					selectAllHostsStmt->execute();
					Poco::Data::RecordSet rs(*selectAllHostsStmt);
					bool more = rs.moveFirst();
					while (more)
					{
						std::size_t cols = rs.columnCount();

						Host* newHost = CreateHostFromDbRow(rs);
						if (newHost == nullptr)
						{
							dmi(L"STORE", L"Unable to load host with id: %i\r\n",
								rs.value(0).extract<int>());
							delete newHost;
							more = rs.moveNext();
							continue;
						}
						//dd(L"STORE", L"Loaded host: %s\r\n",
						//	newHost->to_wstring().c_str());
						hosts.push_back(newHost);
						more = rs.moveNext();
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
				return nullptr;
			}
			// Loads all filesystems from database
			void LoadFileSystems(std::vector<FileSystem*> &file_systems) override
			{
				file_systems.clear();
				if (selectAllFileSystemsStmt == nullptr)
				{
					selectAllFileSystemsStmt = new Poco::Data::Statement(*session);
					*selectAllFileSystemsStmt << selectFileSystemAttributes;
				}
				try
				{
					selectAllFileSystemsStmt->execute();
					Poco::Data::RecordSet rs(*selectAllFileSystemsStmt);
					bool more = rs.moveFirst();
					while (more)
					{
						FileSystem* newFs = CreateFileSystemFromDbRow(rs);
						if (newFs == nullptr)
						{
							dmi(L"STORE", L"Unable to load file system with id: %i\r\n",
								rs.value(0).extract<int>());
							delete newFs;
							more = rs.moveNext();
							continue;
						}
						dd(L"STORE", L"Loaded file system: %s\r\n",
							newFs->to_wstring().c_str());
						file_systems.push_back(newFs);
						more = rs.moveNext();
					}
				}
				catch (...)
				{
					// cleanup partial threshold array
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
			void LoadThresholds(std::vector<Threshold*> &thresholds) override
			{
				thresholds.clear();
				if (selectAllThresholdsStmt == nullptr)
				{
					selectAllThresholdsStmt = new Poco::Data::Statement(*session);
					*selectAllThresholdsStmt << selectThresholdAttributes;
				}
				try
				{
					selectAllThresholdsStmt->execute();
					Poco::Data::RecordSet rs(*selectAllThresholdsStmt);
					bool more = rs.moveFirst();
					while (more)
					{
						Threshold* newThresh = CreateThresholdFromDbRow(rs);
						if (newThresh == nullptr)
						{
							dmi(L"STORE", L"Unable to load threshold with id: %i\r\n",
								rs.value(0).extract<int>());
							delete newThresh;
							more = rs.moveNext();
							continue;
						}
						dd(L"STORE", L"Loaded threshold: %s\r\n",
							newThresh->to_wstring().c_str());
						thresholds.push_back(newThresh);
						more = rs.moveNext();
					}
				}
				catch (...)
				{
					// cleanup partial threshold array
					auto it = thresholds.begin();
					while (it != thresholds.end())
					{
						delete *it;
						it = thresholds.erase(it);
					}

					throw;
				}
			}
			
			// Load config
			void LoadConfiguration() override
			{
				if (selectConfigStmt == nullptr)
				{
					selectConfigStmt = new Poco::Data::Statement(*session);
					*selectConfigStmt << "SELECT [ProxyHostName]"		// 0
						",[SNMPQueueLength]"			// 1
						",[CommandQueueLength]"		// 2
						",[HeartBeatTimeoutSecs]"		// 3
						",[MaxConcurrentCommands]"	// 4
						",[ConfigUpdateIntervalSecs]"	// 5
						",upper(LogMinSeverity)"			// 6
						",cast(Datediff(s, '1970-01-01',ThreshChangeTime) AS bigint)" // 7
						"FROM Proxies",
						Poco::Data::Keywords::into(latestConfig.hostname),
						Poco::Data::Keywords::into(latestConfig.snmpQueueLength),
						Poco::Data::Keywords::into(latestConfig.cmdQueueLength),
						Poco::Data::Keywords::into(latestConfig.heartbeatTimeout),
						Poco::Data::Keywords::into(latestConfig.maxCommands),
						Poco::Data::Keywords::into(latestConfig.updateInterval),
						Poco::Data::Keywords::into(latestConfig.minSeverity),
						Poco::Data::Keywords::into(latestConfig.ftLastThreshChangeTime);
				}
				selectConfigStmt->execute();
			}
			// Update filesystem
			void UpdateFileSystem(FileSystem &fs,
				const wchar_t *whereClause = nullptr) override
			{
				
			}

			void UpdateHost(Host &host) override
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
			int CreateFileSystem(FileSystem &fs) override
			{
				int newId = -1;
				tmpFs = fs;
				if (createFsStmt == nullptr)
				{
					createFsStmt = new Poco::Data::Statement(*session);
					*createFsStmt << createFsSql,
						Poco::Data::Keywords::use(tmpFs.name),
						Poco::Data::Keywords::use(tmpFs.size),
						Poco::Data::Keywords::use(tmpFs.freeMb),
						Poco::Data::Keywords::use(tmpFs.hostId);
				}

				try
				{
					createFsStmt->execute();
					Poco::Data::RecordSet rs(*createFsStmt);
					bool more = rs.moveFirst();
					while (more)
					{
						fs.id = rs.value(0).extract<int>();
						newId = fs.id;
						fs.statusChangeTime = rs.value(1).extract<int64_t>();
						dd(L"STORE", L"Created file system with id: %i, createTime: %I64i\n",
							fs.id,
							fs.statusChangeTime);
						more = rs.moveNext();
					}
				}
				catch (...)
				{
					throw;
				}

				return newId;
			}

			int CreateHost(Host &host) override 
			{
				int newId = -1;
				tmpHost = host;
				if (createHostStmt == nullptr)
				{
					createHostStmt = new Poco::Data::Statement(*session);
					*createHostStmt << createHostSql,
						Poco::Data::Keywords::use(tmpHost.hostname),
						Poco::Data::Keywords::use(tmpHost.ugmonVersion),
						Poco::Data::Keywords::use(tmpHost.ip),
						Poco::Data::Keywords::use(tmpHost.versionString),
						Poco::Data::Keywords::use(tmpHost.osVersionMajor),
						Poco::Data::Keywords::use(tmpHost.osVersionMinor),
						Poco::Data::Keywords::use(tmpHost.osVersionBuild),
						Poco::Data::Keywords::use(tmpHost.servicePack),
						Poco::Data::Keywords::use(tmpHost.architecture),
						Poco::Data::Keywords::use(tmpHost.proxyHostName),
						Poco::Data::Keywords::use(tmpHost.platform);
				}

				try
				{
					createHostStmt->execute();
					Poco::Data::RecordSet rs(*createHostStmt);
					bool more = rs.moveFirst();
					while (more)
					{
						host.id = rs.value(0).extract<int>();
						newId = host.id;
						host.statusChangeTime = rs.value(1).extract<int64_t>();
						dd(L"STORE", L"Created host with id: %i, createTime: %I64i\n",
							host.id,
							host.statusChangeTime);
						more = rs.moveNext();
					}
				}
				catch (...)
				{
					throw;
				}
				
				return newId;
			}
			void CreateThreshold(Threshold &thresh, int evalOrder, bool isLocked) override
			{
				try
				{
					*session << createThresholdSql,
						Poco::Data::Keywords::use(evalOrder),
						Poco::Data::Keywords::use(thresh.hostExprStr),
						Poco::Data::Keywords::use(thresh.volExprStr),
						Poco::Data::Keywords::use(thresh.WarnFreeMegsThresh),
						Poco::Data::Keywords::use(thresh.HighFreeMegsThresh),
						Poco::Data::Keywords::use(thresh.WarnUtilThresh),
						Poco::Data::Keywords::use(thresh.HighUtilThresh),
						Poco::Data::Keywords::use(thresh.WarnSevStr),
						Poco::Data::Keywords::use(thresh.HighSevStr),
						Poco::Data::Keywords::use(thresh.minFSSize),
						Poco::Data::Keywords::use(thresh.maxFSSize),
						Poco::Data::Keywords::use(isLocked),
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception creating threshold: %S\r\n", e.what());
					throw;
				}
			}


			void DeleteFilesystem(int fsId) override
			{
				try
				{
					*session << "DELETE from FileSystems WHERE ID=?",
						Poco::Data::Keywords::use(fsId),
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception deleting filesystem: %S\r\n", e.what());
				}

			}
			void DeleteFilesystemByHostId(int hostId) override
			{
				try
				{
					*session << "DELETE from FileSystems WHERE HostId=?",
						Poco::Data::Keywords::use(hostId),
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception deleting filesystems for hostid: %S\r\n", e.what());
				}
			}
			void DeleteHost(int hostId) override
			{
				DeleteFilesystemByHostId(hostId);
				try
				{
					*session << "DELETE from Hosts WHERE Id=?",
						Poco::Data::Keywords::use(hostId),
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception deleting host: %S\r\n", e.what());
				}
			}
			void DeleteThreshold(int thresholdId) override
			{
				try
				{
					*session << "DELETE from FSThresholds WHERE Id=?",
						Poco::Data::Keywords::use(thresholdId),
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception deleting threshold: %S\r\n", e.what());
				}
			}
			void DeleteAllObjects()
			{
				try
				{
					*session << "DELETE from FileSystems",
						Poco::Data::Keywords::now;
					*session << "DELETE from Hosts",
						Poco::Data::Keywords::now;
					*session << "DELETE from FSThresholds",
						Poco::Data::Keywords::now;
				}
				catch (std::exception &e)
				{
					dc(L"STORE", L"Exception deleting all objects: %S\r\n", e.what());
				}
			}
		private:
			std::string connectionString;
			Host tmpHost;
			FileSystem tmpFs;
			Threshold tmpThresh;
			Poco::Data::Session* session;
			Poco::Data::Statement* selectConfigStmt = nullptr;
			Poco::Data::Statement* selectAllHostsStmt = nullptr;
			Poco::Data::Statement* selectAllFileSystemsStmt = nullptr;
			Poco::Data::Statement* selectAllThresholdsStmt = nullptr;
			Poco::Data::Statement* selectHostByIdStmt = nullptr;
			Poco::Data::Statement* createHostStmt = nullptr;
			Poco::Data::Statement* createFsStmt = nullptr;
			
			FileSystem* CreateFileSystemFromDbRow(Poco::Data::RecordSet& rs)
			{
				FileSystem* newFs = new FileSystem();
				try
				{
					newFs->id = rs.value(0).extract<int>();
					newFs->name = rs.value(1).extract<std::wstring>();
					newFs->size = rs.value(2).extract<int>();
					newFs->freeMb = rs.value(3).extract<int>();
					newFs->status.setStatus(rs.value(4).extract<int>());
					if (rs.value(5).isEmpty())
						newFs->thresholdId = -1;
					else
						newFs->thresholdId = rs.value(5).extract<int>();
					newFs->hostId = rs.value(6).extract<int>();
					newFs->statusChangeTime = rs.value(7).extract<int64_t>();
				}
				catch (...)
				{
					if (newFs != nullptr)
					{
						delete newFs;
						newFs = nullptr;
					}
				}
				return newFs;
			}

			Threshold* CreateThresholdFromDbRow(Poco::Data::RecordSet& rs)
			{
				Threshold* newThresh = new Threshold();
				try
				{
					newThresh->id = rs.value(0).extract<int>();
					newThresh->SetHostExpr(rs.value(1).extract<std::wstring>());
					newThresh->SetVolExpr(rs.value(2).extract<std::wstring>());
					newThresh->WarnFreeMegsThresh = rs.value(3).extract<int>();
					newThresh->HighFreeMegsThresh = rs.value(4).extract<int>();
					newThresh->WarnUtilThresh = rs.value(5).extract<double>();
					newThresh->HighUtilThresh = rs.value(6).extract<double>();
					newThresh->SetWarnSeverity(rs.value(7).extract<std::wstring>().c_str());
					newThresh->SetHighSeverity(rs.value(8).extract<std::wstring>().c_str());
					newThresh->minFSSize = rs.value(9).extract<int>();
					newThresh->maxFSSize = rs.value(10).extract<int>();
				}
				catch(...)
				{
					if (newThresh != nullptr)
					{
						delete newThresh;
						newThresh = nullptr;
					}
				}
				return newThresh;
			}

			Host* CreateHostFromDbRow(Poco::Data::RecordSet& rs)
			{
				Host* newHost = new Host;
				try
				{
					newHost->id = rs.value(0).extract<int>();
					newHost->hostname = rs.value(1).extract<std::wstring>();
					utils::to_lower(newHost->hostname);
					newHost->ip = rs.value(2).extract<std::wstring>();
					if (!rs.value(3).isEmpty())
						newHost->ugmonVersion = rs.value(3).extract<std::wstring>();
					else
						newHost->ugmonVersion = L"Unknown";

					if (!rs.value(4).isEmpty())
						newHost->versionString = rs.value(4).extract<std::wstring>();
					else
						newHost->versionString = L"";
					
					if (!rs.value(5).isEmpty())
						newHost->osVersionMajor = rs.value(5).extract<int>();
					else
						newHost->osVersionMajor = -1;

					if (!rs.value(6).isEmpty())
						newHost->osVersionMinor = rs.value(6).extract<int>();
					else
						newHost->osVersionMinor = -1;

					if (!rs.value(7).isEmpty())
						newHost->osVersionBuild = rs.value(7).extract<std::wstring>();
					else
						newHost->osVersionBuild = L"";

					if (!rs.value(8).isEmpty())
						newHost->servicePack = rs.value(8).extract<int>();
					else
						newHost->servicePack = -1;

					if (!rs.value(9).isEmpty())
						newHost->architecture = rs.value(9).extract<std::wstring>();
					else
						newHost->architecture = L"n/a";
					newHost->platform = rs.value(10).extract<std::wstring>();
					newHost->status.setStatus(newHost->servicePack = rs.value(11).extract<int>());
					newHost->statusChangeTime = rs.value(12).extract<int64_t>();
				}
				catch (...)
				{
					if (newHost != nullptr)
					{
						delete newHost;
						newHost = nullptr;
					}
					throw;
				}
				return newHost;
			}
		};

	}
}