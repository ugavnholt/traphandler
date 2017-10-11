// Correlator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "correlator.h"
#include "service.h"

#ifdef _DEBUG
#ifndef _M_X64 
#include <vld.h>
#endif
#endif

#pragma warning (disable :4996)


int wmain(int argc, wchar_t* argv[])
{
	int retVal = 0;
	// Call loadGlobalStrings, to load the strings we will need from resource file
	LoadGlobalStrings();

	if(argc == 3 && wcsicmp(argv[1], L"-install") == 0)
	{
		if(ServiceInstall(argv[2]))
		{
			PrintError(IDS_SERVICEINSTALLOK);
			goto QUIT_MARK;
		}
		else
		{
			PrintError(IDS_ERROR_SERVICEINSTALLFAIL); // service installation failed
			retVal = 2;
			goto QUIT_MARK;
		}
	} // Install mode
	else if (argc >= 2 && wcsicmp(argv[1], L"-uninstall") == 0)
	{
		if(ServiceDelete())
			goto QUIT_MARK;
		else
		{
			retVal = 2;
			goto QUIT_MARK;
		}
	} // uninstall mode
	else if(argc == 1)
	{
		bDebug=false;		// prevents outputting text to stdout
		DWORD runMode = 0;

		hQuitEvent = CreateEvent(NULL, true, false, NULL);  // create the event signalled by the service handler
															// when we need to shutdown

		Service* pS = (Service*)Service::Create(wcsServiceName, (LPTHREAD_START_ROUTINE)CorrelatorMainFn, &runMode);
		BOOL bOK = pS->Run();
		delete pS;

		if(hQuitEvent != INVALID_HANDLE_VALUE)
			CloseHandle(hQuitEvent);

		goto QUIT_MARK;

	}	// service mode
	else if(argc >= 2 && wcsicmp(argv[1], L"-debug") == 0)
	{
		DWORD runMode = 1;
		bDebug = true;

		// Print the product name and version
		wchar_t *product, *version;
		if(GetVersionInfo(version, product))
		{
			print(L"%s, version %s starting in debug mode, press any key to quit...\r\n", product, version);
		}
		else
			print(L"Unable to get the version info, error in executable\r\n");

		// start our program (runmode tells it that we are in debug mode)
		CorrelatorMainFn(&runMode);
		goto QUIT_MARK;
	}
	else
	{	// Usage
		wchar_t *product, *version;
		
		if(GetVersionInfo(version, product))
		{
			wprintf(L"%s, by Uffe Gavnholt, Rubik solutions, version %s\r\n\r\n", product, version);
		}
		wprintf(L"Usage: %s [-debug|-install <installPath>|-uninstall]", wcsExeFileName);
		wprintf(L"-debug starts the correlator in debug (console mode)\r\n");
		wprintf(L"-install <installPath> installs the correlator service, to start from the path specified\r\n");
		wprintf(L"-uninstall removes the correlator service from the windows service list\r\n");
		retVal = 1;
		goto QUIT_MARK;
	} // usage

QUIT_MARK:

#ifdef _DEBUG
	_getwch();
#endif
	return retVal;
}

