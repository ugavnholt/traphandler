#ifndef __GLOBALCONF__HEAD
#define __GLOBALCONF__HEAD

#include "stdafx.h"
#include "Proxies.h"

static const wchar_t *DBFacility = L"DB    ";

CDataSource _db;	// representation of the database

void DBTimeStampToFileTime(DBTIMESTAMP *dbTime, FILETIME *ft)
{
	SYSTEMTIME sysTime;
	memset(&sysTime, 0, sizeof(SYSTEMTIME));
	sysTime.wYear = dbTime->year;
	sysTime.wMonth = dbTime->month;
	sysTime.wDay = dbTime->day;
	sysTime.wHour = dbTime->hour;
	sysTime.wMinute = dbTime->minute;
	sysTime.wSecond = dbTime->second;
	sysTime.wMilliseconds = (WORD)(dbTime->fraction/1000000);
	

	SystemTimeToFileTime(&sysTime, ft);
}

bool UpdateConfig(CSession *session, bool bCreateConf)
{
	CProxies *ProxyRow = new CProxies();

	bool bConfChanged = false;

	int nEntries=0;

	// Local config variables
	FILETIME ftConfChangeTime;

	ProxyRow->OpenAll(configSession, wcsLocalHostName);
	while(ProxyRow->MoveNext() == S_OK)
	{
		DBTimeStampToFileTime(&ProxyRow->m_ThreshChangeTime, &ftConfChangeTime);
		if(fttoull(ftConfChangeTime) > fttoull(lastConfigUpdateTime))
		{
			if(!bCreateConf)
				dw(DBFacility, L"Database configuration has changed since last check, reloading configuration\r\n");
			memcpy(&lastConfigUpdateTime, &ftConfChangeTime, sizeof(FILETIME));
			bConfChanged = true;

			// update values that doesn't require a restart
			TIMERDURATIONSECS = ProxyRow->m_HeartBeatTimeoutSecs;			// The timer duration for heartbeat timers
			MAXCONCURRENTCMDS = ProxyRow->m_MaxConcurrentCommands;			// maximum number of commands to have active at one time
			THRESHUPDATEINTSECS = ProxyRow->m_ConfigUpdateIntervalSecs;		// The interval between which thresholds are updated if neccesary

			// Valid debug levels:
			// LOG_ALL, LOG_DEBUG, LOG_NORMAL, LOG_WARNING, LOG_MINOR, LOG_MAJOR, LOG_CRITICAL, LOG_EMERGENCY, LOG_NONE
			if(wcsicmp(ProxyRow->m_LogMinSeverity, L"ALL") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to ALL\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_NONE)
					pTrace->StartLogTrace(LOG_ALL, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_ALL, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"DEBUG") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to DEBUG\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_NONE)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_DEBUG, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"NORMAL") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to NORMAL\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_NONE)
					pTrace->StartLogTrace(LOG_NORMAL, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_NORMAL, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"WARNING") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to WARNING\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_WARNING)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_WARNING, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"MINOR") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to MINOR\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_MINOR)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_MINOR, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"MAJOR") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to MAJOR\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_MAJOR)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_MAJOR, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"CRITICAL") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to CRITICAL\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_CRITICAL)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_CRITICAL, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"EMERGENCY") == 0)
			{
				dn(DBFacility, L"Minimum logfile debug level changed to EMERGENCY\r\n");

				if(pTrace->GetLogTraceLevel() == LOG_EMERGENCY)
					pTrace->StartLogTrace(LOG_DEBUG, wcsInstallPath, L"TrapHandler.log", 36000);

				pTrace->SetTraceLevel(LOG_EMERGENCY, LOG_ALL, LOG_ALL, LOG_ALL);
			}
			else if(wcsicmp(ProxyRow->m_LogMinSeverity, L"NONE") == 0)
			{
				dn(DBFacility, L"Log file logging disabled\r\n");
				pTrace->StopLogTrace();
				pTrace->SetTraceLevel(LOG_NONE, LOG_ALL, LOG_ALL, LOG_ALL);
			}

			if(bCreateConf)
			{
				// update values that are not applied yet
				SNMPQUEUELENGTH = ProxyRow->m_SNMPQueueLength;					// buffer up to 1.000.000 snmp traps for processing
				COMMANDQUEUELENGTH = ProxyRow->m_CommandQueueLength;			// How many commands we can queue at one time
			}
		}

		nEntries++;
	}

	ProxyRow->CloseAll();
	ProxyRow->Close();
	delete ProxyRow;

	if(nEntries != 1)
	{
		dc(DBFacility, L"Did not find a database configuration entry for host %s\r\n", wcsLocalHostName);
		return false;
	}

	if(bConfChanged)
		UpdateThresholds(configSession);
	return true;
}

bool InitializeConfig()
{
	memset(&lastConfigUpdateTime, 0, sizeof(FILETIME));
	if(configSession != NULL || metricSession != NULL)
	{
		dma(DBFacility, L"Database already open, connection attempt cancelled\r\n");
		return false;
	}

	HRESULT hr;
	hr = _db.OpenFromInitializationString(wcsConnectionString);	
	if (FAILED(hr))
	{
#ifdef _DEBUG
		AtlTraceErrorRecords(hr);
#endif
		dc(DBFacility, L"Unable to connect to database, error code: 0x%x\r\n", hr);
		return false;
	}

	configSession = new CSession();
	metricSession = new CSession();

	configSession->Open(_db);
	metricSession->Open(_db);

	dd(DBFacility, L"Database connected\r\n");

	// Read the configuration
	return(UpdateConfig(configSession, true));
}

void CleanupConfig()
{
	configSession->Close();
	metricSession->Close();
	delete configSession; configSession = NULL;
	delete metricSession; metricSession = NULL;
}

#endif