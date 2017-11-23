#ifndef __TRACEENGINE_INCLUDE
#define __TRACEENGINE_INCLUDE

//#define ENABLE_NETTRACE

#pragma once

#include <windows.h>
#include <wchar.h>

#ifdef ENABLE_NETTRACE
#include <gcGuid.h>
#pragma comment(lib, "ws2_32.lib")
#else
#define gcGuid void
#endif

// The following defines control which elements of the traceengine is active

/*
#define ENABLE_TRACE
#define ENABLE_TIMERS
#define ENABLE_COUNTERS
#define ENABLE_NETTRACE
#define ENABLE_LOGTRACE
#define ENABLE_CONSTRACE
*/

#define LOG_VOID		-2		// dont't modify current level
#define LOG_ALL			-1		// all logging
#define LOG_DEBUG		0
#define	LOG_NORMAL		1
#define LOG_WARNING		2
#define LOG_MINOR		3
#define LOG_MAJOR		4
#define LOG_CRITICAL	5
#define LOG_EMERGENCY	6
#define	LOG_NONE		255		// no logging

extern volatile long __declspec(align(32))		slTraceEngine;		// Mutex to control access to this object

#ifndef _DEBUG
#define SPINLOCK(x) while (InterlockedCompareExchange(&x, 1, 0) != 0) YieldProcessor();
#define SPINUNLOCK(x) InterlockedDecrement(&x);
#define SPINLOCKT(x,y) while (InterlockedCompareExchange(&x, 1, 0) != 0) YieldProcessor();
#else
#define SPINLOCK(x) while (InterlockedCompareExchange(&x, 1, 0) != 0) YieldProcessor();
#define SPINUNLOCK(x) InterlockedDecrement(&x);
#define SPINLOCKT(x, y) pTraceEngine->StartTimer(y); while (InterlockedCompareExchange(&x, 1, 0) != 0) YieldProcessor(); pTraceEngine->StopTimer(y);
#endif

struct SCounter
{
	wchar_t label[33];
	unsigned int value;
};

struct STimer
{
	wchar_t label[33];
	LARGE_INTEGER startTime;
	LARGE_INTEGER totalTime;
};

struct SMessageHead
{
	unsigned __int32 msgId;
	unsigned __int32 len;
};

class CTraceEngine
{
public:
	// Method to initialize and construct the TraceEngine object
	// if CreateThread is true, logging is performed by a separate thread, and a queue is used
	static CTraceEngine *Initialize(bool CreateThread, unsigned int maxCounters, unsigned int maxTimers, unsigned int maxMsgQLen, unsigned int maxMsgSize);
	static CTraceEngine *GetInstance() { return pTraceEngine; }

	// Disables all traces currently configured
	inline void StopAllTracing()
	{
		if(bConsoleTraceRunning == true)
		{
			StopConsoleTrace();
		}
		if(bLogTraceRunning == true)
		{
			StopLogTrace();
		}
	}

	// Method to create a new debug event
	static void SendEvent(int severity, const wchar_t *facility, const wchar_t *format, ...);

	// Sets the minimum severity of messages logged to any of the destinations
	// -1 = none, 0 = debug, 1 = normal, 2=warning, 3=minor, 4=major, 5=critical, 6=emergency
	inline void SetTraceLevel(int logLevel, int consLevel, int netLevel, int eventlogLevel)
	{
		
		if(logLevel != LOG_VOID)
			iLogTraceLevel = logLevel;
		if(consLevel != LOG_VOID)
			iConsoleTraceLevel = consLevel;

		if(iLogTraceLevel == LOG_NONE)	// logging disabled
			StopLogTrace();
		if(iConsoleTraceLevel == LOG_NONE)
			StopConsoleTrace();
	}

	// Setup a trace file for logging, and start writing alarms to the destination
	// trace level can be: -1 = none, 0 = debug, 1 = normal, 2=warning, 3=minor, 4=major, 5=critical, 6=emergency
	bool StartLogTrace(int traceLevel, const wchar_t *logDir, const wchar_t *fileName, unsigned int MaxAgeMins);

	// Stops logging of trace messages to log file
	void StopLogTrace()
	{
		SPINLOCK(slTraceEngine);
		CloseHandle(pTraceEngine->hLogFile);
		pTraceEngine->hLogFile = INVALID_HANDLE_VALUE;
		pTraceEngine->bLogTraceRunning = false;
		SPINUNLOCK(slTraceEngine);
	}
	inline int GetLogTraceLevel() { return  iLogTraceLevel; }
	inline void FlushLogFile() const 
	{
#ifdef _DEBUG
		SPINLOCKT(slTraceEngine, pTraceEngine->SpinTimer);
#else
		SPINLOCK(slTraceEngine);
#endif
		FlushFileBuffers(pTraceEngine->hLogFile);
		SPINUNLOCK(slTraceEngine);
	}

	// Setup tracing to a console, if one is available
	// trace level can be: -1 = none, 0 = debug, 1 = normal, 2=warning, 3=minor, 4=major, 5=critical, 6=emergency
	bool StartConsoleTrace(int traceLevel);

	// Stops logging to console
	void StopConsoleTrace()
	{
		SPINLOCK(slTraceEngine);
		pTraceEngine->bConsoleTraceRunning = false;
		pTraceEngine->hConsole = INVALID_HANDLE_VALUE;
		SPINUNLOCK(slTraceEngine);
	}
	inline int GetConsoleTraceLevel() { return  iConsoleTraceLevel; }

	/////////////////////////////////
	// Counter handeling methods

	// Creates a counter with the specified label,
	// and returns a HANDLE to identify the created counter (max 32 chars)
	DWORD CreateCounter(const wchar_t *label);
	inline void incCounter(const DWORD counterId) { pTraceEngine->Counters[counterId].value++; }
	inline void decCounter(const DWORD counterId) { pTraceEngine->Counters[counterId].value--; }
	inline unsigned int GetCounter(const DWORD counterId) const { return pTraceEngine->Counters[counterId].value; }
	inline const wchar_t *GetCounterLabel(const DWORD counterId) const { return pTraceEngine->Counters[counterId].label; }
	void PrintAllCountersToLog();
	void PrintAllCountersToConsole();

	//////////////////////////////////////
	// Timer handling functions

	// Creates a timer with the specified label (max 32 chars)
	// and returns handle to the timer
	DWORD CreateTimer(const wchar_t *label);
	inline void StartTimer(const DWORD timerId) { QueryPerformanceCounter(&pTraceEngine->Timers[timerId].startTime); }
	inline void StopTimer(const DWORD timerId) 
	{ 
		LARGE_INTEGER stopTime;
		QueryPerformanceCounter(&stopTime);
		pTraceEngine->Timers[timerId].totalTime.QuadPart = stopTime.QuadPart - pTraceEngine->Timers[timerId].startTime.QuadPart;
	}
	inline LONGLONG GetTimerTime(DWORD timerId) const { return  pTraceEngine->Timers[timerId].totalTime.QuadPart; }
	inline void ResetTimer(DWORD timerId) 
	{ 
		pTraceEngine->Timers[timerId].totalTime.QuadPart = 0;
		pTraceEngine->Timers[timerId].startTime.QuadPart = 0;
	}
	inline const wchar_t*GetTimerLabel(DWORD timerId) const { return pTraceEngine->Timers[timerId].label; }
	inline LONGLONG GetTimerResolution() const { return  pTraceEngine->liTimerResolution.QuadPart; }

	void PrintAllTimersToLog();
	void PrintAllTimersToConsole();

	~CTraceEngine();

private:
	// Private variables
	HANDLE					hThread;		// Handle to output thread (if started)
	DWORD					dwThreadId;		// ID of thread that handles output (if started)
	static CTraceEngine		*pTraceEngine;	// Pointer to the instance of our class
	wchar_t					*msgBuf;		// the buffer to store it in

	// We make our copy contructor private, to prevent copying
	CTraceEngine(const CTraceEngine &te) { }	

	// Our constructor is private, we only want the user to create the object through the 
	// Initialize call
	CTraceEngine()  :
		hThread(INVALID_HANDLE_VALUE), dwThreadId(0),
		iLogTraceLevel(LOG_NONE), bLogTraceRunning(false), wcsLogFile(NULL), ullFileMaxAge(0), 
		iConsoleTraceLevel(LOG_NONE), bConsoleTraceRunning(false),
		dwMaxCounters(0), dwCounters(0), Counters(NULL), dwMaxTimers(0), dwTimers(0), Timers(NULL), uMaxMsgQLen(0), uMaxMsgSize(0), msgBuf(NULL)
	{
	}

	int CTraceEngine::FormatMessage(int severity, const wchar_t *facility, const wchar_t *message);

	static void WriteLogFileMessage(int msgLen);
	static void WriteConsoleMessage(int msgLen);
	DWORD SpinTimer;

	void RollLogFile();

	// Variables used for nettracing
	unsigned int			uMaxMsgQLen;
	unsigned int			uMaxMsgSize;	// max size of a formatted log message

	// Variables used for logTracing
	int						iLogTraceLevel;
	bool					bLogTraceRunning;
	wchar_t					*wcsLogFile;
	HANDLE					hLogFile;
	ULONGLONG				ullLogTime;
	
	ULONGLONG				ullFileMaxAge;			// if we write an entry older than this number of minutes, we roll over

	// variables used for console tracing
	int						iConsoleTraceLevel;
	bool					bConsoleTraceRunning;
	HANDLE					hConsole;

	// variables used for counters
	DWORD					dwMaxCounters;
	DWORD					dwCounters;	// number of counters currently created
	SCounter				*Counters;

	// variables used for timers
	unsigned int			dwMaxTimers;
	unsigned int			dwTimers;	// number of timers currently created
	STimer					*Timers;
	LARGE_INTEGER			liTimerResolution;

};

#endif