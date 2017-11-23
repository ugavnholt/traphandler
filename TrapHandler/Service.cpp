#include "stdafx.h"
#include "utils.h"
#include "Service.h"

// bool queueObj::REQUEST_HALT;
Service* Service::pInstance = 0;

Service::Service(LPTSTR pName, LPTHREAD_START_ROUTINE pThreadFunc, LPVOID pParams) :
hSrv(NULL),
dwCurrState(SERVICE_STOPPED),
srvName(pName),
hThread(NULL),
pThreadFn(pThreadFunc)
{
}

Service *Service::Create(LPTSTR pName, LPTHREAD_START_ROUTINE pThreadFunc, LPVOID pParams)
{
	if(pInstance == 0)
		pInstance = new Service(pName, pThreadFunc, pParams);

	return pInstance;
}

inline const Service* Service::Instance()
{
	return pInstance;
}

Service::~Service()
{
	if (pInstance != NULL)
	{
		//delete pInstance;
		pInstance = NULL;
	}
}

BOOL Service::Run()
{
	SERVICE_TABLE_ENTRY stbl[] = {
		{srvName, reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMainFn)}, {NULL, NULL}
	};

	// Won't return until service has finished executing
	return StartServiceCtrlDispatcher(stbl);
}

void CALLBACK Service::ServiceMainFn(DWORD dwArgc, LPTSTR *lpszArgv)
{
	if(pInstance == NULL)
		return;

	DWORD tid;

	// Register our handler routine with the SCM
	SERVICE_STATUS_HANDLE hService = RegisterServiceCtrlHandler(pInstance->srvName, 
		reinterpret_cast<LPHANDLER_FUNCTION>(SCMHandler));
	if (hService == 0)
		return;
	else
		pInstance->hSrv = hService;

	// Tell the SCM we are starting
	TellSCM(SERVICE_START_PENDING, 0, 1);

	pInstance->OnServiceStart();

	TellSCM(SERVICE_START_PENDING, 0, 2);

	TellSCM(SERVICE_START_PENDING, 0, 3);

	// Ensure that the quit event the thread is waiting for is not
	// signalled
	ResetEvent(hQuitEvent);

	// Create the thread and set it to going

	HANDLE hT = CreateThread(
		0,						// Default security descriptor
		0,						// default stack size
		pInstance->pThreadFn,	// Thread function
		NULL,					// Pinter to parameters
		0,						// Creation flags
		&tid);					// thread ID

	if(tid == 0)
	{
		return;
	}
	else
		pInstance->hThread = hT;

	// Tell the SCM we are ready to go
	TellSCM(SERVICE_RUNNING, 0, 0);

	// Wait for the event to be signalled, which tells us we
	// need to finish
	dd(L"SERV  ", L"Waiting for service thread to terminate\r\n");
	WaitForSingleObject(hQuitEvent, INFINITE);
	dd(L"SERV  ", L"Service thread terminated\r\n");

	// CloseHandle(pInstance->hThread);
	//CloseHandle(pInstance->hQuitEvent); // event handle is closed in main program
}

BOOL Service::TellSCM(DWORD dwState, DWORD dwExitCode, DWORD dwProgress)
{
	// Declare a SERVICE_STATUS structure, and fill it
	SERVICE_STATUS srvStatus;

	srvStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	srvStatus.dwCurrentState = pInstance->dwCurrState = dwState;

	srvStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_ACCEPT_SHUTDOWN;

	srvStatus.dwWin32ExitCode = dwExitCode;
	srvStatus.dwServiceSpecificExitCode = 0;

	srvStatus.dwCheckPoint = dwProgress;
	srvStatus.dwWaitHint = 3000;

	return SetServiceStatus(pInstance->hSrv, &srvStatus);
}

void CALLBACK Service::SCMHandler(DWORD dwCommand)
{
	switch(dwCommand)
	{
	case SERVICE_CONTROL_STOP:
		{
		TellSCM(SERVICE_STOP_PENDING, 0, 1);

		// call the stopping method
		pInstance->OnServiceStopping();
		bDoShutdown = true;
		
		DWORD dwWaitResult = WaitForSingleObject(pInstance->hThread,10000); // Give the worker thread 10 secs to exit
		if(dwWaitResult == WAIT_TIMEOUT)
			dw(L"SERV  ", L"Timeout waiting for thread to terminate\r\n");

		// Set event, to say we are stopped
		SetEvent(hQuitEvent);	// signal the main thread that we are quitting		

		pInstance->OnServiceStopped();

		// Tell SCM we are stopped
		TellSCM(SERVICE_STOPPED, 0, 0);
		break;
		}

	case SERVICE_CONTROL_PAUSE:
		TellSCM(SERVICE_PAUSE_PENDING, 0, 1);

		pInstance->OnServicePausing();

		// Pause worker thread
		SuspendThread(pInstance->hThread);

		pInstance->OnServicePaused();

		TellSCM(SERVICE_PAUSED, 0, 0);
		break;

	case SERVICE_CONTROL_CONTINUE:
		TellSCM(SERVICE_CONTINUE_PENDING, 0, 1);

		pInstance->OnServiceResuming();

		ResumeThread(pInstance->hThread);

		pInstance->OnServiceResumed();

		TellSCM(SERVICE_RUNNING, 0, 0);
		break;

	case SERVICE_CONTROL_INTERROGATE:
		TellSCM(pInstance->dwCurrState, 0, 0);
		break;

	case SERVICE_CONTROL_SHUTDOWN:
		// Do whatever we need to do before shutdown
		break;
	}
}

bool Service::Install(LPCTSTR pName, LPCTSTR pDisplayName, LPCTSTR exePath)
{
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if(hSCM == NULL)
		return false;
	
	SC_HANDLE hService = CreateService(hSCM,
		pName,
		pDisplayName,
		SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		exePath,
		NULL, NULL, NULL, NULL, NULL);

	if (hService == NULL)
	{
		CloseServiceHandle(hSCM);
		return false;
	}
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);

	return true;
}

bool Service::Delete(LPCTSTR pName)
{
	SC_HANDLE hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if(hSCM == NULL)
		return false;
	
	SC_HANDLE hSrv = OpenService(hSCM, pName, SERVICE_ALL_ACCESS);
	if(hSrv == NULL)
	{
		CloseServiceHandle(hSCM);
		return false;
	}

	if(! DeleteService(hSrv))
	{
		CloseServiceHandle(hSrv);
		CloseServiceHandle(hSCM);
		return false;
	}

	CloseServiceHandle(hSrv);
	CloseServiceHandle(hSCM);
	return true;
}

// Overrideable functions
void Service::OnServiceStart()
{
}

void Service::OnServiceStopping()
{
}

void Service::OnServiceStopped()
{
}

void Service::OnServicePausing()
{
}

void Service::OnServicePaused()
{
}

void Service::OnServiceResuming()
{
}

void Service::OnServiceResumed()
{
}