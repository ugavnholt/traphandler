// stdafx.cpp : source file that includes just the standard includes
// TrapHandler.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=UGMon;Data Source=UG-M1730\\SQLEXPRESS;Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=UG-M1730;Use Encryption for Data=False;Tag with column collation when possible=False";
wchar_t *wcsConnectionString = NULL;
wchar_t *wcsInstallPath = NULL;
const wchar_t *wcsTrapHandlerRegKey=L"SOFTWARE\\Rubik Solutions\\TrapHandler";
CSession *configSession = NULL;
CSession *metricSession = NULL;
FILETIME lastConfigUpdateTime;

int SNMPQUEUELENGTH=1000000;	// buffer up to 1.000.000 snmp traps for processing
int COMMANDQUEUELENGTH=10000;	// How many commands we can queue at one time
int TIMERDURATIONSECS=10;		// The timer duration for heartbeat timers
int MAXCONCURRENTCMDS=5;		// maximum number of commands to have active at one time
int THRESHUPDATEINTSECS=5;		// The interval between which thresholds are updated if neccesary

FILETIME LastHBTime;	// Time of last activity, used for heart beat

BOOL bDebug = true;
BOOL bDoShutdown = false;	// Global shutdown flag
HANDLE hQuitEvent = INVALID_HANDLE_VALUE;	// global handle to our quit flag
CTraceEngine *pTrace = NULL;

wchar_t *wcsapp(wchar_t *str1, const wchar_t *str2)
{
	size_t str2len = wcslen(str2)+1;
	memcpy(str1, str2, str2len*sizeof(wchar_t));
	return str1+str2len-1;
}

const wchar_t *cTrapHandler = L"TrapHandler";