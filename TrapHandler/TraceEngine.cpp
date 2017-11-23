#include "stdafx.h"
#include "TraceEngine.h"

CTraceEngine*	CTraceEngine::pTraceEngine = 0;
volatile long __declspec(align(32))	slTraceEngine = 1;
DWORD WINAPI TraceEngineThreadFn( LPVOID pArg);
static const DWORD INVALID_VALUE = 0xFFFFFFFF;
static const unsigned short unicodeTag = 0xfeff;	// the tag we write to a unicode file

static const wchar_t *wcsDebug =  L"DBUG-";
static const wchar_t *wcsNormal = L"NORM-";
static const wchar_t *wcsWarning = L"WARN-";
static const wchar_t *wcsMinor = L"MINO-";
static const wchar_t *wcsMajor = L"MAJO-";
static const wchar_t *wcsCritical = L"CRIT-";

bool bDebug = false;
CTraceEngine *pTrace = nullptr;

#pragma warning (disable : 4996)

int CTraceEngine::FormatMessage(int severity, const wchar_t *facility, const wchar_t *message)
{
	SYSTEMTIME stNow;
	GetSystemTime(&stNow);

	const wchar_t *sev = L"UNKNOWN";
	if (severity == LOG_DEBUG)
		sev = wcsDebug;
	else if (severity = LOG_NORMAL)
		sev = wcsNormal;
	else if (severity = LOG_WARNING)
		sev = wcsWarning;
	else if (severity = LOG_MINOR)
		sev = wcsMinor;
	else if (severity = LOG_MAJOR)
		sev = wcsMajor;
	else if (severity = LOG_CRITICAL)
		sev = wcsCritical;

	int strLen = swprintf(pTraceEngine->msgBuf, pTraceEngine->uMaxMsgSize+50, L"%04u%02u%02u %02u:%02u:%02u %s%s: %s\r\n", 
		stNow.wYear, stNow.wMonth, stNow.wDay, stNow.wHour, stNow.wMinute, stNow.wSecond, sev, facility, message  );

	return strLen;
}

bool CTraceEngine::StartConsoleTrace(int traceLevel)
{
	if(pTraceEngine->bConsoleTraceRunning)
		return false;

	pTraceEngine->iConsoleTraceLevel = traceLevel;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if(hConsole == INVALID_HANDLE_VALUE || hConsole == NULL)
		return false;
	pTraceEngine->bConsoleTraceRunning = true;
	return true;
}


void CTraceEngine::WriteConsoleMessage(int msgLen)
{
	DWORD dwWritten;

	char *tmpBuf = new char[msgLen+1];
	CharToOemW(pTraceEngine->msgBuf, tmpBuf);

	WriteFile(pTraceEngine->hConsole, tmpBuf, (DWORD)msgLen, &dwWritten, NULL);
	delete [] tmpBuf;
}

void CTraceEngine::WriteLogFileMessage(int msgLen)
{
	DWORD dwWritten;
	WriteFile(pTraceEngine->hLogFile, pTraceEngine->msgBuf, (DWORD)msgLen*sizeof(wchar_t), &dwWritten, NULL);
}


void CTraceEngine::RollLogFile()
{
	// Check if we need to roll the log file
	FILETIME ftNow;
	GetSystemTimeAsFileTime(&ftNow);
	//wprintf(L"%x%x - %I64x\n", ftNow.dwHighDateTime, ftNow.dwLowDateTime, pTraceEngine->ullLogTime);
	
	ULONGLONG ullNow = ftNow.dwHighDateTime;
	ullNow = (ullNow << 32) + ftNow.dwLowDateTime;
	//wprintf(L"%I64x\n", ullNow);

	ULONGLONG ullDelta = ullNow-ullLogTime;

	if((ullNow-ullLogTime) >= (pTraceEngine->ullFileMaxAge) )
	{
		//wprintf(L"delta %I64x maxAge: %I64x\n", ullDelta, pTraceEngine->ullFileMaxAge);
		// We need to move our current file to a .bak backup
		CloseHandle(pTraceEngine->hLogFile);
		wchar_t *oldFileName = new wchar_t[wcslen(pTraceEngine->wcsLogFile)+10];
		wcscpy(oldFileName, pTraceEngine->wcsLogFile);
		wcscat(oldFileName, L".old");

		MoveFileEx(pTraceEngine->wcsLogFile, oldFileName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
		delete [] oldFileName;

		hLogFile = CreateFile(pTraceEngine->wcsLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
		if(hLogFile == INVALID_HANDLE_VALUE)
		{
			pTraceEngine->bLogTraceRunning = false;	// we can't run without a file
			return;
		}
		DWORD dwWritten;
		WriteFile(hLogFile, &unicodeTag, sizeof(unsigned short), &dwWritten, NULL);

		pTraceEngine->ullLogTime = ullNow;
	}
}

bool CTraceEngine::StartLogTrace(int traceLevel, const wchar_t *logDir, const wchar_t *fileName, unsigned int MaxAgeMins)
{
	if(pTraceEngine == NULL || pTraceEngine->bLogTraceRunning)
		return false;
	pTraceEngine->ullFileMaxAge = MaxAgeMins;
	pTraceEngine->ullFileMaxAge = pTraceEngine->ullFileMaxAge * 600000000;
	pTraceEngine->wcsLogFile = new wchar_t[wcslen(logDir)+wcslen(fileName)+2]; // room for \0 and \\.
	wcscpy(pTraceEngine->wcsLogFile, logDir);
	wcscat(pTraceEngine->wcsLogFile, L"\\");
	wcscat(pTraceEngine->wcsLogFile, fileName);

	pTraceEngine->iLogTraceLevel = traceLevel;

	// Check if the logfile we need exist
	hLogFile = CreateFile(pTraceEngine->wcsLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
	if(hLogFile == INVALID_HANDLE_VALUE)
	{
		hLogFile = CreateFile(pTraceEngine->wcsLogFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
		if(hLogFile == INVALID_HANDLE_VALUE)
			return false;	// unable to create log file

		// Write unicode tag to log file;
		DWORD dwWritten;
		WriteFile(hLogFile, &unicodeTag, sizeof(unsigned short), &dwWritten, NULL);
	}
	// Save the timestamp of our log file
	FILETIME ftLogFileTime;
	GetFileTime(pTraceEngine->hLogFile, &ftLogFileTime, NULL, NULL);

	pTraceEngine->ullLogTime = ftLogFileTime.dwHighDateTime;
	pTraceEngine->ullLogTime = (pTraceEngine->ullLogTime<<32)+ftLogFileTime.dwLowDateTime;

	
	// Call RollLogFile to ensure that the timestamps are updatéd,
	// and that we start on a new log file if neccesary
	pTraceEngine->RollLogFile();
	
	SetFilePointer(hLogFile, 0, 0, FILE_END);

	pTraceEngine->bLogTraceRunning = true;
	return true;
}

void CTraceEngine::SendEvent(int severity, const wchar_t *facility, const wchar_t *format, ...)
{
	if(pTraceEngine == NULL)
		return;

	if(	severity < pTraceEngine->GetConsoleTraceLevel() &&
		severity < pTraceEngine->GetLogTraceLevel())
	{
		return;
	}

	int msgLen;
	const wchar_t *pSev = L"UNKNOWN";
	wchar_t *pTmp;
	if(severity == LOG_DEBUG)
		pSev = wcsDebug;
	else if(severity == LOG_NORMAL)
		pSev = wcsNormal;
	else if(severity == LOG_WARNING)
		pSev = wcsWarning;
	else if(severity == LOG_MINOR)
		pSev = wcsMinor;
	else if(severity == LOG_MAJOR)
		pSev = wcsMajor;
	else if(severity == LOG_CRITICAL)
		pSev = wcsCritical;

	SYSTEMTIME stNow;
	GetSystemTime(&stNow);

#ifdef _DEBUG
	SPINLOCKT(slTraceEngine, pTraceEngine->SpinTimer);
#else
	SPINLOCKT(slTraceEngine, 0);
#endif

	msgLen = swprintf(pTraceEngine->msgBuf, 50, L"%02u-%02u %02u:%02u:%02u %s%s: ", 
		stNow.wMonth, stNow.wDay, stNow.wHour, stNow.wMinute, stNow.wSecond, pSev, facility);

	pTmp = pTraceEngine->msgBuf+msgLen;

	// Format the log string
	va_list args;
	va_start (args, format);
	msgLen += vswprintf(pTmp, pTraceEngine->uMaxMsgSize, format, args);
	va_end(args);

	// Do we write the events directly?
	if(pTraceEngine->hThread == INVALID_HANDLE_VALUE)
	{
		if(pTraceEngine->bLogTraceRunning && severity >= pTraceEngine->iLogTraceLevel)
			WriteLogFileMessage(msgLen);
		if(pTraceEngine->bConsoleTraceRunning && severity >= pTraceEngine->iConsoleTraceLevel)
		{
			WriteConsoleMessage(msgLen);
		}
	}
	else
	{	// create a message event, and put it into the queue for the output processor
	}

	SPINUNLOCK(slTraceEngine);
}

DWORD CTraceEngine::CreateCounter(const wchar_t *label)
{
	if(pTraceEngine->dwCounters >= pTraceEngine->dwMaxCounters)
		return INVALID_VALUE;

	wcsncpy(pTraceEngine->Counters[pTraceEngine->dwCounters].label, label, 32);
	pTraceEngine->Counters[pTraceEngine->dwCounters].label[32] = L'\0';
	pTraceEngine->Counters[pTraceEngine->dwCounters].value = 0;

	dwCounters++;
	return dwCounters-1;
}

DWORD CTraceEngine::CreateTimer(const wchar_t *label)
{
	if(pTraceEngine->dwTimers >= pTraceEngine->dwMaxTimers)
		return INVALID_VALUE;

	wcsncpy(pTraceEngine->Timers[pTraceEngine->dwTimers].label, label, 32);
	pTraceEngine->Timers[pTraceEngine->dwTimers].label[32] = L'\0';
	pTraceEngine->Timers[pTraceEngine->dwTimers].startTime.QuadPart = 0;
	pTraceEngine->Timers[pTraceEngine->dwTimers].totalTime.QuadPart = 0;

	dwTimers++;
	return dwTimers-1;
}
#pragma warning (default :4996)

CTraceEngine *CTraceEngine::Initialize(bool bCreateThread, unsigned int maxCounters, unsigned int maxTimers, unsigned int maxMsgQLen, unsigned int maxMsgSize)
{
	if(CTraceEngine::pTraceEngine != 0)
		return NULL;	// we only initialize once

	pTraceEngine = new CTraceEngine();

	// SPINLOCK(slTraceEngine); - we don't need this, we initialize the variable to 1 (locked)

	// Initialize the message buffer
	pTraceEngine->uMaxMsgSize = maxMsgSize;
	pTraceEngine->msgBuf = new wchar_t [maxMsgSize+50]; // we add 32 chars for facility, and 

	// Initialize the counter buffer
	pTraceEngine->dwMaxCounters = maxCounters;
	pTraceEngine->Counters = new SCounter[maxCounters];
	QueryPerformanceFrequency(&pTraceEngine->liTimerResolution);

	// Initialize the timer buffer
	pTraceEngine->dwMaxTimers = maxTimers;
	pTraceEngine->Timers = new STimer[maxTimers];

#ifdef _DEBUG
	pTraceEngine->SpinTimer = pTraceEngine->CreateTimer(L"slTraceEngine");
	//wprintf(L"Spintimerid :%u\n", pTraceEngine->SpinTimer);
#endif

	// initialize the message queue
	pTraceEngine->uMaxMsgQLen = maxMsgQLen;

	if(bCreateThread)
	{
		pTraceEngine->hThread = CreateThread(NULL, 0, TraceEngineThreadFn, 0, 0, &pTraceEngine->dwThreadId);
	}

	SPINUNLOCK(slTraceEngine);
	return pTraceEngine;
}

CTraceEngine::~CTraceEngine()
{
#ifdef _DEBUG
	wprintf(L"Time spend in spinlock: %I64ims\n", pTraceEngine->GetTimerTime(pTraceEngine->SpinTimer) /(pTraceEngine->GetTimerResolution() / 1000));
#endif
	// Shutdown the worker thread
	if(hThread != INVALID_HANDLE_VALUE)
	{
		// Instruct our worker to quit;
		PostThreadMessage(pTraceEngine->dwThreadId, WM_QUIT, 0, 0);

		// Wait for it to happend
		WaitForSingleObject(pTraceEngine->hThread, INFINITE);
		CloseHandle(pTraceEngine->hThread);
		pTraceEngine->hThread = INVALID_HANDLE_VALUE;
	}

	// Shutdown tracing
	pTraceEngine->StopAllTracing();
	
	// Lock our object
	SPINLOCK(slTraceEngine);

	if(pTraceEngine->Counters != NULL)
		delete [] pTraceEngine->Counters;

	if(pTraceEngine->Timers != NULL)
		delete [] pTraceEngine->Timers;
	if(pTraceEngine->msgBuf != NULL)
		delete [] pTraceEngine->msgBuf;
	
	if(pTraceEngine->wcsLogFile != NULL)
		delete [] wcsLogFile;

	// SPINUNLOCK(slTraceEngine); Leave spinlock locked, we assume that when we construct the object
	pTraceEngine = NULL;
}

DWORD WINAPI TraceEngineThreadFn( LPVOID pArg)
{
	MSG		msg;
	BOOL	MsgReturn = true;

	while(MsgReturn)	// MsgReturn can be false if WM_QUIT is received
	{
		MsgReturn = GetMessage( &msg ,NULL ,0 , 0 );
            
		if ( MsgReturn )
		{
			switch ( msg.message )
			{
			case WM_QUIT:
				// We are requested to shut down
				goto THREADEXIT;
				break;
			} // WM_QUIT
		}	// We had a message

    }	// while(1)

THREADEXIT:

	return 0;
}