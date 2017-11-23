#include "stdafx.h"
#include "service.h"
#include <vld.h>

#include "TrapHandler.h"

#pragma warning (disable :4996)

int wmain(int argc, wchar_t* argv[])
{
	CoInitialize(NULL);
	int retVal = 0;
	// initialize the global instance
	traphandler::TrapHandler::Initialize();
	// get a reference to the global instance
	traphandler::TrapHandler &app = traphandler::TrapHandler::GetInstance();


	if(argc == 5 && wcsicmp(argv[1], L"-install") == 0)
	{
		if(app.ServiceInstall(argv[2]))
		{
			app.PrintError(IDS_SERVICEINSTALLOK);

			// Create the connection string
			if(app.CreateConf(argv[2], argv[3], argv[4]))
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
			app.PrintError(IDS_ERROR_SERVICEINSTALLFAIL); // service installation failed
			retVal = 2;
			goto QUIT_MARK;
		}
	} // Install mode
	else if (argc >= 2 && wcsicmp(argv[1], L"-uninstall") == 0)
	{
		if(app.ServiceDelete())
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
		
		if(!app.ReadConf())
		{
			wprintf(L"Unable to read configuration, there might be an installation problem, or database is unavailable\r\n");
			goto QUIT_MARK;
		}

		pTrace = CTraceEngine::Initialize(false, 10, 10, 0, 4096);
		pTrace->StartLogTrace(LOG_NORMAL, wcsInstallPath, L"TrapHandler.log", 36000);

		// Print the product name and version
		dn(Facility, L"%s, version %s starting in service mode...\r\n", app.productName, app.versionString);
		
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
		if(!app.ReadConf())
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
		if(app.GetVersionInfo(version, product))
		{
			print(Facility, L"%s, version %s starting in debug mode, press any key to quit...\r\n", product, version);
		}
		else
			print(Facility, L"Unable to get the version info, error in executable\r\n");

		// start our program (runmode tells it that we are in debug mode)
		app.HandlerMainFn(&runMode);

		delete pTrace; pTrace = NULL;
		CleanupConfig();
		goto QUIT_MARK;
	}
	else
	{	// Usage
		wchar_t *product, *version;
		
		if(app.GetVersionInfo(version, product))
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

namespace traphandler
{
	inline void TrapHandler::processTraps()
	{
		SSnmpPacket *newPacket = nullptr;
		while (pQueue.try_pop(newPacket))
		{
			bool bError = false;
			// Decode the trap
			CSnmpTrap snmpTrap(newPacket);
			if (!snmpTrap.Decode(newPacket->buf))
			{
				dmi(L"SNMP  ", L"Received malformed SNMP packet\r\n");
				delete newPacket;
				continue;
			}
			if ((*snmpTrap.pReqType & 0xff) == 0xA6)
				trapReceiver.SendAck(newPacket, snmpTrap.pReqType);

			///////////////////////////
			// Process known SNMP traps here
			traphandler::events::AgentEvent *newEvent = nullptr;
			if (!eventFactory.GetEventFromTrap(snmpTrap, newEvent))
			{
				std::wstring msg = L"Received unknown SNMP event with oid: ";
				msg += (const wchar_t*)snmpTrap.TrapOid;
				dmi(L"SNMP  ", L"%s \r\n", msg.c_str());
				delete newPacket;
				continue;
			}
			if (!newEvent->ParseTrap(snmpTrap))
			{
				dmi(L"SNMP  ", L"Received malformed SNMP event\r\n");
				delete newPacket;
				continue;
			}
			dd(L"SNMP  ", L"Received event: %s\r\n", newEvent->to_wstring());

			std::wstring cmd = newEvent->ProcessTrap(model);
			if (!cmd.empty())
				cmdQueue.QueueCommand(cmd);
			if (!timers.TouchTimer(newEvent->hostname, newEvent->platformId, TIMERDURATIONSECS))
			{
				dd(Facility, L"Created timer %s:%s, duration %i\r\n",
					newEvent->hostname,
					newEvent->platformId,
					TIMERDURATIONSECS);
			}
			else
			{
				dd(Facility, L"Updating timer: %s:%s\r\n",
					newEvent->hostname,
					newEvent->platformId);
			}
		}
	}

			/*
			////////////////////////////
			// Disk Metric event
			if (snmpTrap.TrapOid == &TrapOidDiskMetric)
			{
				wchar_t *timerId = new wchar_t[300];
				wchar_t *p = timerId;
				p = wcsapp(timerId, pHostName);
				p = wcsapp(p, L":");
				p = wcsapp(p, pPlatformID);
				// update or create timer for host
				FILETIME ftNow;
				GetSystemTimeAsFileTime(&ftNow);
				TimerObj *timer = LocateTimer(timerId, heartBeatExpired);
				if (timer == NULL)
				{
					timer = new TimerObj(&ftNow, TIMERDURATIONSECS, timerId, heartBeatExpired);
					ActiveTimers.push_back(timer);
					dd(Facility, L"Creating timer %s, duration %i\r\n", timerId, TIMERDURATIONSECS);
				}
				else
				{
					timer->Update(&ftNow, TIMERDURATIONSECS);
					dd(Facility, L"Updating timer: %s\r\n", timerId);
				}

				delete[] timerId;

				__int64 TotDiskSpace = 0, FreeDiskSpace = 0;
				TotDiskSpace = _wtoi64(pTotSpace);
				FreeDiskSpace = _wtoi64(pFreeSpace);

				double utilPercent = -1.0f;
				if (TotDiskSpace > 0)
				{
					// double utilPercent = 100.0f - ( ((double)FreeDiskSpace/(double)TotDiskSpace) * 100.0f );
					utilPercent = (double)FreeDiskSpace;
					utilPercent /= TotDiskSpace;
					utilPercent *= 100.0f;
					utilPercent = 100.0f - utilPercent;
				}

				// find a matching file system if there is one
				CFS *pFS = GetFSByName(pHostName, pFSName);
				if (pFS == NULL)
				{	// we didn't find the file system, lets create a new one
					pFS = new CFS(pHostName,
						pFSName,
						pPlatformID,
						TotDiskSpace,
						FreeDiskSpace,
						utilPercent);
					FileSystems.push_back(pFS);

					// wprintf(L"util: %.2f\r\n", utilPercent);

					dd(Facility, L"Created entry for filesystem %s on host %s\r\n", pFSName, pHostName);
				}
				else
					pFS->SetValues(TotDiskSpace, FreeDiskSpace, utilPercent);
				// wprintf(L"Total disk space: %I64i, Free space: %I64i, util: %f\r\n", TotDiskSpace, FreeDiskSpace, utilPercent);

				bError = true;	// we don't have an error condition, but we don't want to forward this trap

			} // Disk metric event
			else
			{
				dmi(L"SNMP  ", L"Received incorrectly formatted disk metric event\r\n");
				bError = true;
			}
		}
		////////////////////////////
		// Process Fail event
		else if (*snmpTrap->TrapOid == &TrapOidProcFail)
		{
			// queue the command for execution
			CmdQueue.push(newCmd);
			dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
			delete[] wcsCmd;
		}	// process state message
		else
		{
			dmi(L"SNMP  ", L"Received invalid formatted process state change event\r\n");
			bError = true;
		}
	}
	////////////////////////////
	// Maintenance start event
		else if (*snmpTrap->TrapOid == &TrapOidStartMaint)
		{
			wchar_t *timerId = new wchar_t[300];
			wchar_t *p = timerId;
			p = wcsapp(timerId, pHostName);
			p = wcsapp(p, L":");
			p = wcsapp(p, pPlatformID);

			// update or create timer for host
			FILETIME ftNow;
			GetSystemTimeAsFileTime(&ftNow);
			TimerObj *timer = LocateTimer(timerId, maintExpired);
			if (timer == NULL)
			{
				int dur = _wtoi(pDuration) * 60;
				dd(Facility, L"Creating timer %s, duration %i seconds\r\n", timerId, dur);
				timer = new TimerObj(&ftNow, dur, timerId, maintExpired);
				ActiveTimers.push_back(timer);
			}
			else
			{
				dd(Facility, L"Updating timer: %s\r\n", timerId);
				timer->Update(&ftNow, TIMERDURATIONSECS);
			}
			delete[] timerId;
		}
		else
		{
			dmi(L"SNMP  ", L"Received invalid formatted begin maintenance event\r\n");
			bError = true;
		}
}

////////////////////////////
// Hello event
		else if (*snmpTrap->TrapOid == &TrapOidHello)
		{
			wchar_t *timerId = new wchar_t[300];
			wchar_t *p = timerId;
			p = wcsapp(timerId, pHostName);
			p = wcsapp(p, L":");
			p = wcsapp(p, pPlatformID);

			// update or create timer for host
			FILETIME ftNow;
			GetSystemTimeAsFileTime(&ftNow);
			TimerObj *timer = LocateTimer(timerId, heartBeatExpired);
			if (timer == NULL)
			{
				dd(Facility, L"Creating timer %s, duration: %i\r\n", timerId, TIMERDURATIONSECS);
				timer = new TimerObj(&ftNow, TIMERDURATIONSECS, timerId, heartBeatExpired);
				ActiveTimers.push_back(timer);
			}
			else
			{
				dd(Facility, L"Updating timer: %s\r\n", timerId);
				timer->Update(&ftNow, TIMERDURATIONSECS);
			}

			delete[] timerId;

			// Update host in host discovery table
			UpdateHost(configSession, pHostName, pPlatformID, pOSFlavor, pUgmonVersion, pSourceIP, wcsLocalHostName);

			// queue the command for execution
			CmdQueue.push(newCmd);
			dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
			delete[] wcsCmd;
		}   // Hostname seems ok
			}
			else
			{
				dmi(L"SNMP  ", L"Received invalid hello message\r\n");
				bError = true;
			}

		}
		////////////////////////////
		// Goodbye event
		else if (*snmpTrap->TrapOid == &TrapOidGoodbye)
		{
			wchar_t *timerId = new wchar_t[300];
			wchar_t *p = timerId;
			p = wcsapp(timerId, pHostName);
			p = wcsapp(p, L":");
			p = wcsapp(p, pPlatformID);

			//wprintf(L"deleting timer %s\r\n", snmpTrap->varArgs[0]->wcsVal);
			dd(Facility, L"Delting timer %s\r\n", timerId);
			DeleteTimer(timerId, heartBeatExpired);

			delete[] timerId;

			// Construct the forward command

			ExecObj *newCmd = new ExecObj();
			newCmd->setCmdStr(wcsCmd);

			// queue the command for execution
			CmdQueue.push(newCmd);
			dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
			delete[] wcsCmd;
		}
		else
		{
			dmi(L"SNMP  ", L"Received invalid goodbye message\r\n");
			bError = true;
		}
		}
		else
		{
			dd(L"SNMP  ", L"Received unknown trap: %s\r\n", (wchar_t*)*snmpTrap->TrapOid);
		}

		////////////////////////////
		// Generic event
		if (!bError)
		{
			////////////////////////////////
			// Format trap to OMW message, and schedule it to be forwarded to OMW
			ExecObj *newCmd = new ExecObj();
			wchar_t *cmd = new_;
			newCmd->setCmdStr(cmd);

			// queue the command for execution
			CmdQueue.push(newCmd);
			dd(L"SNMP  ", L"Queued command: %s\r\n", cmd);
			// We are done with the trap
			delete[] cmd;
		}
		delete newPacket;
	}
}*/

// initializes the application running as a service
bool TrapHandler::StartAsService()
{
}
// initializes the application running in the console
bool TrapHandler::StartAsConsole()
{
}
void TrapHandler::Shutdown()
{
}

inline bool TrapHandler::Initialize()
{
	if (pInstance == nullptr)
		return true;	// already constructed
	pInstance = new TrapHandler();

	pInstance->LoadGlobalStrings();
	if (!pInstance->GetVersionInfo())
	{
		delete pInstance;
		pInstance = nullptr;
		return false;
	}
	return true;
}

inline void TrapHandler::LoadGlobalStrings()
{
	hInst = GetModuleHandle(NULL);
	LoadString(hInst, IDS_EXEFILENAME, wcsExeFileName, 255);
	LoadString(hInst, IDS_SERVICESHORTNAME, wcsServiceName, 255);
	LoadString(hInst, IDS_SERVICENAME, wcsServiceLabel, 255);
	LoadString(hInst, IDS_SERVICEDESCR, wcsServiceDescr, 2048);

	DWORD dwSize = 255;
	GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, wcsLocalHostName, &dwSize);
	wcslwr(wcsLocalHostName);
	// wprintf(L"running on %s\r\n", wcsLocalHostName);
}

// Prints an error message loaded from the resource file
inline void TrapHandler::PrintError(int IDSString)
{
	if (hInst != 0)
	{
		wchar_t wcsError[2049];
		LoadString(hInst, IDSString, wcsError, 2048);
		wprintf(L"%s\r\n", wcsError);
	}
	else
		wprintf(L"Unable to print error string, resource file not loaded\r\n");
}

bool TrapHandler::GetVersionInfo()
{
	// load and lock the version resource 
	HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	HGLOBAL hGlobal = ::LoadResource(NULL, hRsrc);
	LPVOID pVersionResource = ::LockResource(hGlobal);
	if (pVersionResource == NULL)
	{
		return false;
	}

	// get the name and version strings 
	unsigned int iProductNameLen = 0;
	unsigned int iProductVersionLen = 0;

	// replace "040904e4" with the language ID of your resources 
	if (!VerQueryValue(pVersionResource, L"\\StringFileInfo\\040904b0\\ProductName", (LPVOID*)productName, &iProductNameLen) ||
		!VerQueryValue(pVersionResource, L"\\StringFileInfo\\040904b0\\ProductVersion", (LPVOID*)versionString, &iProductVersionLen))
	{
		return false;
	}

	// wprintf(L"Product: %s, version: %s\r\n", productName, versionString);
	return true;
}

// Main program loop
inline DWORD TrapHandler::HandlerMainFn(LPDWORD pParam)
{
	TrapHandler app = TrapHandler::GetInstance();
	if (!app.consoleMode && app.hQuitEvent == INVALID_HANDLE_VALUE)
	{
		dc(Facility, L"TrapHandler not initialized in debug mode, but quit event is not initialized\r\n");
	}
	DWORD dwWaitTime = 100;

	DWORD dwWaitResult = WAIT_TIMEOUT;
	BOOL bQuit = false;
	FILETIME ftLastThreshUpdateTime;
	
	GetSystemTimeAsFileTime(&ftLastThreshUpdateTime);
	uint64_t lastThreshUpdateTime = utils::fttoull(ftLastThreshUpdateTime);
	
	app.trapReceiver = new CTrapReceiver(162, SNMPQUEUELENGTH);
	app.trapReceiver->Start();
	dd(Facility, L"SNMP trap receiver started\r\n");

	while (!bQuit)
	{
		dwWaitResult = WAIT_TIMEOUT;
		if (app.cmdQueue.isEmpty())	// only wait if there is no traps to process
			dwWaitResult = WaitForSingleObject(app.hQuitEvent, dwWaitTime);

		// Do we have to quit?
		if (bDoShutdown || ((!app.consoleMode && dwWaitResult == WAIT_OBJECT_0) || (app.consoleMode && _kbhit() != 0)))
		{
			if (dwWaitResult == WAIT_OBJECT_0)
			{
				dd(Facility, L"Quit flag signalled\r\n");
			}
			else
				SetEvent(app.hQuitEvent);
			if (app.consoleMode && _kbhit())
				_getwch();
			break;
		}
		else
		{
			///////////////////////////////
			// Update thresholds
			FILETIME ftNow;
			uint64_t now = utils::fttoull(ftNow);
			GetSystemTimeAsFileTime(&ftNow);

			// has the threshold update interval elapsed?
			if ((now - lastThreshUpdateTime) > ((ULONGLONG)THRESHUPDATEINTSECS*(ULONGLONG)FTCLICKSQERSEC))
			{
				//dd(Facility, L"UpdateInterval\r\n");
				// dd(Facility, L"Updating thresholds\r\n");
				lastThreshUpdateTime = now;
				
				///////////////////////////////
				// Check configuration for updates
				app.UpdateConfig(configSession, false);

				dn(Facility, L"System status: Currently %u commands are active, out of maximum %u\r\n", nCommands, MAXCONCURRENTCMDS);
			}

			///////////////////////////////
			// Pop all SNMP traps from queue
			app.processTraps(trapReceiver, eventFactory);

			//////////////////////////////
			// Check status of runningCommands
			app.cmdQueue.processCommands();

			///////////////////////////////
			// Check all timers

			auto timIt = app.timers.ActiveTimers.begin();
			while (timIt != app.timers.ActiveTimers.end())
			{
				if ((*timIt)->isExpired(&now))
				{
					dd(Facility, L"Timer %s expired, duration was: %I64u\r\n", (*timIt)->GetTimerID(), (*timIt)->GetDuration());

					// build and queue timer command
					std::wstring cmd(L"opcmsg a=UGMon o=Heart-beat -node=");
					cmd += (*timIt)->GetTimerID();

					//wcscat(cmd, (*timIt)->GetTimerID());
					if ((*timIt)->GetTimerType() == model::heartBeatExpired)
					{
						cmd += L" msg_t=\"No heart-beat received from node: ";
						cmd += (*timIt)->GetTimerID();
					}
					else if ((*timIt)->GetTimerType() == model::maintExpired)
					{
						cmd += L" msg_t=\"Maintenance has ended for node: ";
						cmd += (*timIt)->GetTimerID();
					}
					cmd += L"\" -option hostname=";
					cmd += (*timIt)->GetTimerID();
					
					app.cmdQueue.QueueCommand(cmd);
					
					delete (*timIt);
					timIt = ActiveTimers.erase(timIt);
				}
				else
					timIt++;
			}

			///////////////////////////////
			// Execute any commands queued (if we have available command slots)
			cmdQueue.ProcessQueue();

		}
	} // main loop
	conn.disconnect();

	app.trapReceiver->Stop();

	// Stop SNMP trap engine, and empty queue
	dd(Facility, L"Stopping SNMP trap receiver, received %u trap(s)\r\n", trapReceiver->GetTrapCount());
	unsigned int trapPopCount = 0;

	SSnmpPacket *newPacket;
	while (pQueue.try_pop(newPacket))
	{
		delete newPacket;
		trapPopCount++;
	}
	delete app.trapReceiver;
	app.trapReceiver = nullptr;

	if (trapPopCount > 0)
		dw(Facility, L"Due to application shutdown, %u SNMP trap(s) were dropped\r\n", trapPopCount);

	// TODO save timers and host state so that we can reload them from database
	// Save any running timers, and delete the timer list
	dd(Facility, L"%u timer(s) active upon application shutdown, saving state...\r\n", app.timers.size());
	
	
	print(Facility, L"Executed %u command(s)\r\n", cmdQueue.GetExecutedCommands());

	print(Facility, L"Main loop terminated\r\n");
	return 0;
}

// Program configuration reader
inline bool TrapHandler::ReadConf()
{
	// Read the connection string from registry
	HKEY hKey;
	if (RegOpenKey(HKEY_LOCAL_MACHINE, wcsTrapHandlerRegKey, &hKey) == ERROR_SUCCESS)
	{
		if (wcsInstallPath != NULL)
			delete[] wcsInstallPath;

		wcsInstallPath = new wchar_t[MAX_PATH + 1];
		DWORD dwSize = MAX_PATH * sizeof(wchar_t);
		DWORD regType;

		if (RegQueryValueEx(hKey, L"Install Path", 0, &regType, (BYTE*)wcsInstallPath, &dwSize) != ERROR_SUCCESS)
		{
			wprintf(L"unable to read install path\r\n");
			RegCloseKey(hKey);
			return false;
		}

		if (wcsConnectionString != NULL)
			delete[] wcsConnectionString;

		wcsConnectionString = new wchar_t[65536];
		dwSize = 65535 * sizeof(wchar_t);
		if (RegQueryValueEx(hKey, L"Connection String", 0, &regType, (BYTE*)wcsConnectionString, &dwSize) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return false;
		}

		dwSize = sizeof(DWORD);
		if (RegQueryValueEx(hKey, L"Grace period seconds", 0, &regType, (BYTE*)&dwGraceTimeSecs, &dwSize) != ERROR_SUCCESS)
		{
			wprintf(L"DWORD Registry key 'Grace period seconds' not defined\r\n");
			RegCloseKey(hKey);
			return false;
		}

		RegCloseKey(hKey);
	}
	else
		return false;

	return true;
}

// Configuration builder
inline bool TrapHandler::CreateConf(const wchar_t *installPath, const wchar_t *instanceName, const wchar_t *dbName)
{
	HKEY hKey;
	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, wcsTrapHandlerRegKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		if (RegSetValueEx(hKey, L"Install Path", 0, REG_SZ, (BYTE*)installPath, (DWORD)(wcslen(installPath) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			// wprintf(L"Unable to set install path\r\n");
			return false;
		}

		// build the connection string

		if (wcsConnectionString != NULL)
			delete[] wcsConnectionString;

		wcsConnectionString = new wchar_t[65536];

		wcscpy(wcsConnectionString, L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=");
		wcscat(wcsConnectionString, dbName);
		wcscat(wcsConnectionString, L";Data Source=");
		wcscat(wcsConnectionString, instanceName);
		wcscat(wcsConnectionString, L";Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=");

		wcscat(wcsConnectionString, wcsLocalHostName);
		wcscat(wcsConnectionString, L";Use Encryption for Data=False;Tag with column collation when possible=False");


		if (RegSetValueEx(hKey, L"Connection String", 0, REG_SZ, (BYTE*)wcsConnectionString, (DWORD)(wcslen(wcsConnectionString) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			return false;
		}

		RegCloseKey(hKey);
	}
	else
		return false;

	return true;
}

bool TrapHandler::ServiceDelete()
{
	PrintError(IDS_DELETESERVICEMSG);

	if (!Service::Delete(wcsServiceName))
	{
		wprintf(L"Service failed to uninstall\r\n");
		return false;
	}
	wprintf(L"Service successfully uninstalled\r\n");
	return true;
}

bool TrapHandler::ServiceInstall(const wchar_t *pathName)
{
	size_t dirLength = wcslen(pathName);
	PrintError(IDS_INSTALLMSG);
	// construct the binary path name
	wchar_t binName[MAX_CMD];
	wcscpy(binName, pathName);
	if (binName[wcslen(binName) - 1] != L'\\')
		wcscat(binName, L"\\");

	wcscat(binName, wcsExeFileName);

	// Check wether the file exist
	HANDLE hFile = CreateFile(binName, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		if (err == 0x02 || err == 0x03)	// File not found
			PrintError(IDS_ERROR_INCORRECTPATH);
		else
			PrintError(IDS_ERROR_UNKNOWN_FILEMSG);
#ifdef _DEBUG
		wprintf(L"CreateFile failed with error: %u\r\n", err);
#endif
		return false;
	}
	CloseHandle(hFile);
	// We have the correct path, lets create our service
	if (!Service::Install(wcsServiceName, wcsServiceLabel, binName))
	{
		PrintError(IDS_ERROR_SERVICEINST);
		return false;
	}

	// Service should now be installed, lets configure the registry DB correctly
	wchar_t RegKeyPath[MAX_PATH + 1];
	wcscpy(RegKeyPath, L"SYSTEM\\CurrentControlSet\\Services\\");
	wcscat(RegKeyPath, wcsServiceName);
	// wprintf(L"RegDB: %s\n", RegKeyPath);
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyPath, 0, KEY_WRITE | KEY_READ, &hKey) != ERROR_SUCCESS)
	{
		wprintf(L"Unable to open service registry database\r\n");
		ServiceDelete();
		return false;
	}

	DWORD dwStartMode = 0x02;
	if (RegSetValueEx(hKey, L"Start", 0, REG_DWORD, (BYTE*)&dwStartMode, sizeof(DWORD)) != ERROR_SUCCESS)
	{
		wprintf(L"Unable to set service start mode to automatic\r\n");
		RegCloseKey(hKey);
		ServiceDelete();
		return false;
	}

	if (RegSetValueEx(hKey, L"Description", 0, REG_SZ, (BYTE*)&wcsServiceDescr, (DWORD)(wcslen(wcsServiceDescr) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
	{
		wprintf(L"Unable to set service description\r\n");
		RegCloseKey(hKey);
		ServiceDelete();
		return false;
	}

	RegCloseKey(hKey);

	return true;
}
} // namespace traphandler

