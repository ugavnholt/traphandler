// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#include <windows.h>
#include <wchar.h>
#include <conio.h>
#include "resource.h"
#include "TraceEngine.h"
#include <atldbcli.h>

// config values
extern int SNMPQUEUELENGTH;			// buffer up to 1.000.000 snmp traps for processing
extern int COMMANDQUEUELENGTH;		// How many commands we can queue at one time
extern int TIMERDURATIONSECS;		// The timer duration for heartbeat timers
extern int MAXCONCURRENTCMDS;		// maximum number of commands to have active at one time
extern int THRESHUPDATEINTSECS;		// The interval between which thresholds are updated if neccesary

static volatile int LastEntryPoint = 0;				// ID of the last entry point function
extern FILETIME LastHBTime;	// Time of last activity, used for heart beat

#define FTCLICKSQERSEC 10000000		// how many 100ns intervals in one second
#define MAXTRAPSTRLEN 65535			// the maximum string length of a trap when converted
#define MAX_CMD 2048				// the maximum length of a command line string

extern FILETIME lastConfigUpdateTime;
extern CSession *configSession;
extern CSession *metricSession;
extern const wchar_t *wcsTrapHandlerRegKey;
extern wchar_t *wcsConnectionString;
extern wchar_t *wcsInstallPath;
extern BOOL bDebug;
extern BOOL bDoShutdown;
extern CTraceEngine *pTrace;
extern wchar_t *wcsapp(wchar_t *str1, const wchar_t *str2);
extern HANDLE hQuitEvent;	// global handle to our quit flag
							// global flag, determines whether we should print output

// #define print(x,...) if(bDebug) {_print(x,__VA_ARGS__);}
#define print(x,z,...) pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);
inline void _print(const wchar_t *format,...)
{
	if(!bDebug)
		return;

	va_list argList;
	va_start(argList, format);

	vwprintf(format, argList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug macros
// z = message
#define dd(x,z,...)	pTrace->SendEvent(LOG_DEBUG, x, z, __VA_ARGS__);
#define dn(x,z,...)	pTrace->SendEvent(LOG_NORMAL, x, z, __VA_ARGS__);
#define dw(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);
#define dmi(x,z,...)	pTrace->SendEvent(LOG_MINOR, x, z, __VA_ARGS__);
#define dma(x,z,...)	pTrace->SendEvent(LOG_MAJOR, x, z, __VA_ARGS__);
#define dc(x,z,...)	pTrace->SendEvent(LOG_WARNING, x, z, __VA_ARGS__);
