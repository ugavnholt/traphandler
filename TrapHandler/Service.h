#ifndef __SERVICE_HEAD
#define __SERVICE_HEAD

#include "stdafx.h"


class Service
{
	SERVICE_STATUS_HANDLE hSrv;
	DWORD dwCurrState;
	LPTSTR srvName;
	HANDLE hThread;
	LPTHREAD_START_ROUTINE pThreadFn;

	static Service * pInstance;

	static BOOL TellSCM(DWORD dwState, DWORD dwExitCode, DWORD dwProgress);

	// Private copy constructor to prohibit copying
	Service(const Service &s) {}

protected:
	// Only create service object through the Create Function
	Service(LPTSTR pName, LPTHREAD_START_ROUTINE pThreadFunc, LPVOID pParams);

public:
	static Service * Create(LPTSTR pName, LPTHREAD_START_ROUTINE pThreadFunc, LPVOID pParams = 0);

	// Return pointer to static instance
	static const Service* Instance();

	// Virtual destructor
	virtual ~Service();

	// Callbacks for SCM
	static void CALLBACK SCMHandler(DWORD dwCommand);
	static void CALLBACK ServiceMainFn(DWORD dwArgc, LPTSTR* lpszArgv);

	// Non-static functions
	BOOL Run();

	// Static installation and deletion functions
	static bool Install(LPCTSTR pName,			// The name of the service
						LPCTSTR pDisplayName,	// The name displayed for the service
						LPCTSTR exePath);		// Path name to the service executive
	static bool Delete(LPCTSTR pName);

	// Overridables to hook into service code

virtual void OnServiceStart();
virtual void OnServiceStopping();
virtual void OnServiceStopped();
virtual void OnServicePausing();
virtual void OnServicePaused();
virtual void OnServiceResuming();
virtual void OnServiceResumed();
};

#endif