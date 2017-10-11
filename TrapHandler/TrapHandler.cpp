#include "stdafx.h"
#include "service.h"

#include "TrapHandler.h"

#pragma warning (disable :4996)

int wmain(int argc, wchar_t* argv[])
{
	CoInitialize(NULL);
	int retVal = 0;

	// Call loadGlobalStrings, to load the strings we will need from resource file
	LoadGlobalStrings();

	if(argc == 5 && wcsicmp(argv[1], L"-install") == 0)
	{
		if(ServiceInstall(argv[2]))
		{
			PrintError(IDS_SERVICEINSTALLOK);

			
			// Create the connection string
			if(CreateConf(argv[2], argv[3], argv[4]))
			{
				wprintf(L"Configuration successfully created\r\n");
				wprintf(L"remember to configure the service \"%s\" to logon as a user with dbo rights to the %s database\r\n", wcsServiceLabel, argv[4]);
			}
			else
				wprintf(L"There was an error creating registry values for %s\r\n", wcsServiceLabel);
		
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
		{
			HKEY hKey;
			if(RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Rubik Solutions", &hKey) == ERROR_SUCCESS)
			{
				RegDeleteKey(hKey, L"TrapHandler");
				RegCloseKey(hKey);
			}

			goto QUIT_MARK;
		}
		else
		{
			retVal = 2;
			goto QUIT_MARK;
		}
	} // uninstall mode
	else if(argc == 1)
	{
		
		if(!ReadConf())
		{
			wprintf(L"Unable to read configuration, there might be an installation problem, or database is unavailable\r\n");
			goto QUIT_MARK;
		}

		pTrace = CTraceEngine::Initialize(false, 10, 10, 0, 4096);
		pTrace->StartLogTrace(LOG_NORMAL, wcsInstallPath, L"TrapHandler.log", 36000);

		// Print the product name and version
		wchar_t *product, *version;
		if(GetVersionInfo(version, product))
		{
			dn(Facility, L"%s, version %s starting in service mode...\r\n", product, version);
		}
		else
			dn(Facility, L"Unable to get the version info, error in executable\r\n");

		if(!InitializeConfig())
		{
			dc(L"MAIN  ", L"Unable to initialize database, shutting down\r\n");
			delete pTrace; pTrace = NULL;
			CleanupConfig();
			goto QUIT_MARK;
		}

		bDebug=false;		// prevents outputting text to stdout
		DWORD runMode = 0;

		hQuitEvent = CreateEvent(NULL, true, false, NULL);  // create the event signalled by the service handler
															// when we need to shutdown

		Service* pS = (Service*)Service::Create(wcsServiceName, (LPTHREAD_START_ROUTINE)HandlerMainFn, &runMode);
		BOOL bOK = pS->Run();
		delete pS;

		if(hQuitEvent != INVALID_HANDLE_VALUE)
			CloseHandle(hQuitEvent);

		delete pTrace; pTrace = NULL;
		CleanupConfig();
		goto QUIT_MARK;

	}	// service mode
	else if(argc >= 2 && wcsicmp(argv[1], L"-debug") == 0)
	{
		if(!ReadConf())
		{
			wprintf(L"Unable to read configuration, there might be an installation problem, or database is unavailable\r\n");
			goto QUIT_MARK;
		}

		pTrace = CTraceEngine::Initialize(false, 10, 10, 0, 4096);
		pTrace->StartConsoleTrace(LOG_ALL);
		pTrace->StartLogTrace(LOG_NORMAL, wcsInstallPath, L"TrapHandler.log", 36000);
		
		if(!InitializeConfig())
		{
			dc(L"MAIN  ", L"Unable to initialize database, shutting down\r\n");
			delete pTrace; pTrace = NULL;
			CleanupConfig();
			goto QUIT_MARK;
		}

		DWORD runMode = 1;
		bDebug = true;

		// Print the product name and version
		wchar_t *product, *version;
		if(GetVersionInfo(version, product))
		{
			print(Facility, L"%s, version %s starting in debug mode, press any key to quit...\r\n", product, version);
		}
		else
			print(Facility, L"Unable to get the version info, error in executable\r\n");

		// start our program (runmode tells it that we are in debug mode)
		HandlerMainFn(&runMode);

		delete pTrace; pTrace = NULL;
		CleanupConfig();
		goto QUIT_MARK;
	}
	else
	{	// Usage
		wchar_t *product, *version;
		
		if(GetVersionInfo(version, product))
		{
			wprintf(L"%s, by Uffe Gavnholt, Rubik solutions, version %s\r\n\r\n", product, version);
		}
		wprintf(L"Usage: %s [-debug|-install|-uninstall]", wcsExeFileName);
		wprintf(L"-debug starts the correlator in debug (console mode)\r\n");
		wprintf(L"-install <installPath> <DBInstance> <DBName> installs the correlator service, to start from the path specified\r\n");
		wprintf(L"-uninstall removes the correlator service from the windows service list\r\n");
		retVal = 1;
		goto QUIT_MARK;
	} // usage

QUIT_MARK:

#ifdef _DEBUG
    wprintf(L"Application terminated, press any key...");
	_getwch();
    wprintf(L"\n");
#endif
	if(wcsConnectionString != NULL) delete [] wcsConnectionString;
	if(wcsInstallPath != NULL) delete [] wcsInstallPath;
	CoUninitialize();
	return retVal;
}

