#ifndef __CORRELATOR_HEAD
#define __CORRELATOR_HEAD

#include "TraceEngine.h"
#include "service.h"
#include <list>
#include "TrapReceiver.h"
#include "TimerObj.h"
#include "HostControl.h"
#include "utils.h"
#include "AgentEventFactory.h"
#include "TrapHandlerModel.h"
#include "CmdQueue.h"

#pragma comment (lib, "Version.lib") // to read the VS_VERSION_INFO structure
#pragma warning (disable :4996)

static const wchar_t *Facility		= L"MAIN  ";
const wchar_t *NoPlatform			= L"UNKNOWN";
const wchar_t *VersionPre11			= L"pre-1.11";
const wchar_t *DefaultMsgGrp		= L"OS_";

const wchar_t *SevNormal			= L"Normal";
const wchar_t *SevWarning			= L"Warning";
const wchar_t *SevMinor				= L"Minor";
const wchar_t *SevMajor				= L"Major";
const wchar_t *SevCritical			= L"Critical";
const wchar_t *DefaultSeverity		= SevMajor;

traphandler::CmdQueue cmdQueue;
std::list<TimerObj*> ActiveTimers;				// The timers currently active

#include "Thresholds.h"

////////////////////////////////
// Globals

HINSTANCE hInst = 0;
wchar_t wcsExeFileName[256];
wchar_t wcsServiceName[256];
wchar_t wcsServiceLabel[256];
wchar_t wcsServiceDescr[2049];
wchar_t wcsLocalHostName[256];	// our local host name

#include "globalConf.h"
////////////////////////////////

DWORD WINAPI queueFunc(LPVOID p);
void processTraps(CTrapReceiver *trapReceiver,
	traphandler::events::AgentEventFactory &eventFactory,
	traphandler::model::TrapHandlerModel &model);

inline void processTraps(CTrapReceiver *trapReceiver, 
	traphandler::events::AgentEventFactory &eventFactory,
	traphandler::model::TrapHandlerModel &model)
{
	SSnmpPacket *newPacket = nullptr;
	while(pQueue.try_pop(newPacket))
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
		if((*snmpTrap.pReqType & 0xff) == 0xA6)
			trapReceiver->SendAck(newPacket, snmpTrap.pReqType);

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

		// Call the queue, to process the trap, and queue any actions needed
		cmdQueue.QueueCommand(*newEvent, model);

        ////////////////////////////
        // Disk Metric event
		if(snmpTrap.TrapOid == &TrapOidDiskMetric)
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
				if(timer == NULL)
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

                delete [] timerId;

				__int64 TotDiskSpace = 0, FreeDiskSpace = 0;
				TotDiskSpace = _wtoi64(pTotSpace);
				FreeDiskSpace = _wtoi64(pFreeSpace);

				double utilPercent = -1.0f;
				if(TotDiskSpace > 0)
				{
					// double utilPercent = 100.0f - ( ((double)FreeDiskSpace/(double)TotDiskSpace) * 100.0f );
					utilPercent = (double)FreeDiskSpace;
					utilPercent /= TotDiskSpace;
					utilPercent *= 100.0f;
					utilPercent = 100.0f - utilPercent;
				}

				// find a matching file system if there is one
				CFS *pFS = GetFSByName(pHostName, pFSName);
				if(pFS == NULL)
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
					pFS->SetValues(TotDiskSpace, FreeDiskSpace,	utilPercent);
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
		else if(*snmpTrap->TrapOid == &TrapOidProcFail)
		{
				// queue the command for execution
				CmdQueue.push(newCmd);
				dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
				delete [] wcsCmd;
			}	// process state message
			else
			{
				dmi(L"SNMP  ", L"Received invalid formatted process state change event\r\n");
				bError = true;
			}
		}
        ////////////////////////////
        // Service fail event
		else if(*snmpTrap->TrapOid == &TrapOidServFail)
		{
				// queue the command for execution
				CmdQueue.push(newCmd);
				dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
				delete [] wcsCmd;
			}
            else
            {
				dmi(L"SNMP  ", L"Received invalid formatted service state change event\r\n");
				bError = true;
			}
			
		}
        ////////////////////////////
        // Cluster resource group fail event
		else if(*snmpTrap->TrapOid == &TrapOidClusFail)
		{
			
		}
        ////////////////////////////
        // Maintenance start event
		else if(*snmpTrap->TrapOid == &TrapOidStartMaint)
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
				if(timer == NULL)
				{
					int dur = _wtoi(pDuration)*60;
					dd(Facility, L"Creating timer %s, duration %i seconds\r\n", timerId, dur);
					timer = new TimerObj(&ftNow, dur, timerId, maintExpired);
					ActiveTimers.push_back(timer);
				}
				else
				{
					dd(Facility, L"Updating timer: %s\r\n", timerId);
					timer->Update(&ftNow, TIMERDURATIONSECS);
				}
                delete [] timerId;
			}
            else
            {
				dmi(L"SNMP  ", L"Received invalid formatted begin maintenance event\r\n");
				bError = true;
			}
		}
        ////////////////////////////
        // Error message event
		else if(*snmpTrap->TrapOid == &TrapOidError)
		{

		}
        ////////////////////////////
        // Hello event
		else if(*snmpTrap->TrapOid == &TrapOidHello)
		{
			if((snmpTrap->varArgs.size() == 1 && 
				snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR) ||
                (snmpTrap->varArgs.size() == 4 && 
				snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR && 
                snmpTrap->varArgs[1]->iValueType == SNMP_TYPE_STR &&
                snmpTrap->varArgs[2]->iValueType == SNMP_TYPE_STR &&
                snmpTrap->varArgs[3]->iValueType == SNMP_TYPE_STR))
			{
                const wchar_t *pPlatformID = NoPlatform;
                const wchar_t *pOSFlavor = L"No OS Information - Old version of UGMon";
                wchar_t *pHostName, *pSourceIP;
                const wchar_t *pUgmonVersion = VersionPre11;
                if(snmpTrap->varArgs.size() == 1)
                {
                    pHostName = snmpTrap->varArgs[0]->wcsVal;
                }
                else
                {
                    pHostName = snmpTrap->varArgs[0]->wcsVal;
                    pPlatformID = snmpTrap->varArgs[1]->wcsVal;
                    pOSFlavor = snmpTrap->varArgs[2]->wcsVal;
                    pUgmonVersion = snmpTrap->varArgs[3]->wcsVal;
                }

                if(*pHostName == L' ' || *pHostName == L'\0')
                {
                    dmi(Facility, L"Received invalid hostname in hello message: '%s'\r\n", pHostName);
                }
                else
                {

                    // Format the SNMPSourceIP as a string
                    pSourceIP = new wchar_t[16];
                
                    swprintf(pSourceIP, 16, L"%u.%u.%u.%u",
                        (snmpTrap->ulSourceIP & 0xff000000) >> 24,
                        (snmpTrap->ulSourceIP & 0x00ff0000) >> 16,
                        (snmpTrap->ulSourceIP & 0x0000ff00) >> 8,
                        (snmpTrap->ulSourceIP & 0x000000ff));

				    dd(L"SNMP  ", L"Received hello from %s(%s) IP: (%s) - %s, running ugmon version: %s\r\n", 
					    pHostName,
                        pPlatformID,
                        pSourceIP,
                        pOSFlavor,
                        pUgmonVersion);

                    wchar_t *timerId = new wchar_t[300];
                    wchar_t *p = timerId;
                    p = wcsapp(timerId, pHostName);
                    p = wcsapp(p, L":");
                    p = wcsapp(p, pPlatformID);

				    // update or create timer for host
				    FILETIME ftNow;
				    GetSystemTimeAsFileTime(&ftNow);
				    TimerObj *timer = LocateTimer(timerId, heartBeatExpired);
				    if(timer == NULL)
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

                    delete [] timerId;

                    // Update host in host discovery table
                    UpdateHost(configSession, pHostName, pPlatformID, pOSFlavor, pUgmonVersion, pSourceIP, wcsLocalHostName);

                    // Format and send message
                    bError = true;
                    wchar_t *wcsCmd = new wchar_t[MAX_CMD+1];
				    p = wcsCmd;
				    p = wcsapp(p, L"opcmsg a=UGMon o=helloMessage -node=");
				    p = wcsapp(p, pHostName);
				    p = wcsapp(p, L" msg_t=\"host='");
				    p = wcsapp(p, pHostName);
				    p = wcsapp(p, L"' platform='");
				    p = wcsapp(p, pPlatformID);
				    p = wcsapp(p, L"' OSFlavor='");
				    p = wcsapp(p, pOSFlavor);
                    p = wcsapp(p, L"' SourceIP='");
				    p = wcsapp(p, pSourceIP);
                    p = wcsapp(p, L"' UGMonVer='");
				    p = wcsapp(p, pUgmonVersion);
				    p = wcsapp(p, L"'\"");
				    ExecObj *newCmd = new ExecObj();
				    newCmd->setCmdStr(wcsCmd);

                    delete [] pSourceIP;
				
				    // queue the command for execution
					CmdQueue.push(newCmd);
					dd(L"SNMP  ", L"Queued command: %s\r\n", wcsCmd);
				    delete [] wcsCmd;
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
		else if(*snmpTrap->TrapOid == &TrapOidGoodbye)
		{
			if((snmpTrap->varArgs.size() == 1 && 
				snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR) || 
                (snmpTrap->varArgs.size() == 4 && 
				snmpTrap->varArgs[0]->iValueType == SNMP_TYPE_STR &&
                snmpTrap->varArgs[1]->iValueType == SNMP_TYPE_STR &&
                snmpTrap->varArgs[2]->iValueType == SNMP_TYPE_STR &&
                snmpTrap->varArgs[3]->iValueType == SNMP_TYPE_STR))
			{
                const wchar_t *pPlatformID = NoPlatform, *pUgmonVersion = VersionPre11, *pOSVersion = L"No OS Information - Old version of UGMon";
                wchar_t *pHostName = snmpTrap->varArgs[0]->wcsVal;
                if(snmpTrap->varArgs.size() == 4)
                {
                    pPlatformID = snmpTrap->varArgs[1]->wcsVal;
                    pOSVersion = snmpTrap->varArgs[2]->wcsVal;
                    pUgmonVersion = snmpTrap->varArgs[3]->wcsVal;
                }

				dd(L"SNMP  ", L"Received Goodbye from %s(%s)\r\n", 
					pHostName,
                    pPlatformID);

                wchar_t *timerId = new wchar_t[300];
                wchar_t *p = timerId;
                p = wcsapp(timerId, pHostName);
                p = wcsapp(p, L":");
                p = wcsapp(p, pPlatformID);

				//wprintf(L"deleting timer %s\r\n", snmpTrap->varArgs[0]->wcsVal);
				dd(Facility, L"Delting timer %s\r\n", timerId);
				DeleteTimer(timerId, heartBeatExpired);

                delete [] timerId;

				// Construct the forward command
				 wchar_t *wcsCmd = new wchar_t[MAX_CMD+1];
				p = wcsCmd;
				p = wcsapp(p, L"opcmsg a=UGMon o=goodbyeMessage -node=");
				p = wcsapp(p, pHostName);
				p = wcsapp(p, L" msg_t=\"host='");
				p = wcsapp(p, pHostName);
				p = wcsapp(p, L"' platform='");
				p = wcsapp(p, pPlatformID);
                p = wcsapp(p, L"' UGMonVer='");
				p = wcsapp(p, pUgmonVersion);
				p = wcsapp(p, L"'\"");
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
		if(!bError)
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
			delete [] cmd;
		}
		delete newPacket;
	}
}

void LoadGlobalStrings()
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
void PrintError(int IDSString)
{
	if(hInst != 0)
	{
		wchar_t wcsError[2049];
		LoadString(hInst, IDSString, wcsError, 2048);
		wprintf(L"%s\r\n", wcsError);
	}
	else
		wprintf(L"Unable to print error string, resource file not loaded\r\n");
}

bool GetVersionInfo(wchar_t *&versionString, wchar_t *&productName)
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
    if (!VerQueryValue(pVersionResource, L"\\StringFileInfo\\040904b0\\ProductName", (LPVOID*)&productName, &iProductNameLen) || 
        !VerQueryValue(pVersionResource, L"\\StringFileInfo\\040904b0\\ProductVersion", (LPVOID*)&versionString, &iProductVersionLen)) 
    {  
        return false; 
    }

	// wprintf(L"Product: %s, version: %s\r\n", productName, versionString);
	return true;
}

// Main program loop
DWORD HandlerMainFn(LPDWORD pParam)
{
	if(!bDebug && hQuitEvent == INVALID_HANDLE_VALUE)
	{
		dc(Facility, L"TrapHandler not initialized in debug mode, but quit event is not initialized\r\n");
	}
	DWORD dwWaitTime = 100;
	traphandler::events::AgentEventFactory eventFactory;

	LoadHosts(configSession);

	DWORD dwWaitResult = WAIT_TIMEOUT;
	BOOL bQuit = false;
	FILETIME ftLastThreshUpdateTime;
	GetSystemTimeAsFileTime(&ftLastThreshUpdateTime);

	//connection(const string& connection_string, long timeout = 0);
	nanodbc::connection conn(std::string(
		"Driver={SQL Server};Server=.\testdb;Database=traphandler"
		"Trusted_Connection = Yes; "))
	traphandler::model::TrapHandlerModel model(conn);

	CTrapReceiver *trapReceiver = new CTrapReceiver(162, SNMPQUEUELENGTH);
	trapReceiver->Start();
	dd(Facility, L"SNMP trap receiver started\r\n");
	
	while(!bQuit)
	{
		dwWaitResult = WAIT_TIMEOUT;
		if(cmdQueue.isEmpty())	// only wait if there is no traps to process
			dwWaitResult = WaitForSingleObject(hQuitEvent, dwWaitTime);

		// Do we have to quit?
		if(bDoShutdown || ( (!bDebug && dwWaitResult == WAIT_OBJECT_0) || (bDebug && _kbhit() != 0) ))
		{
			if(dwWaitResult == WAIT_OBJECT_0)
			{
				dd(Facility, L"Quit flag signalled\r\n");
			}
			else
				SetEvent(hQuitEvent);
			if(bDebug && _kbhit())
				_getwch();
			break;
		}
		else
		{
			///////////////////////////////
			// Update thresholds
			FILETIME ftNow;
			GetSystemTimeAsFileTime(&ftNow);

			// has the threshold update interval elapsed?
			if((fttoull(ftNow)-fttoull(ftLastThreshUpdateTime)) > ((ULONGLONG)THRESHUPDATEINTSECS*(ULONGLONG)FTCLICKSQERSEC))
			{
				//dd(Facility, L"UpdateInterval\r\n");
				// dd(Facility, L"Updating thresholds\r\n");
				memcpy(&ftLastThreshUpdateTime, &ftNow, sizeof(FILETIME));

				///////////////////////////////
				// Check configuration for updates
				UpdateConfig(configSession, false);

				dn(Facility, L"System status: Currently %u commands are active, out of maximum %u\r\n", nCommands, MAXCONCURRENTCMDS);
			}

			///////////////////////////////
			// Pop all SNMP traps from queue
			processTraps(trapReceiver, eventFactory);

			//////////////////////////////
			// Check status of runningCommands
			cmdQueue.processCommands();

			///////////////////////////////
			// Check all timers
			GetSystemTimeAsFileTime(&ftNow);
			std::list<TimerObj*>::iterator timIt = ActiveTimers.begin();
			while (timIt != ActiveTimers.end())
			{
				if((*timIt)->isExpired(&ftNow))
				{
					dd(Facility, L"Timer %s expired, duration was: %I64u\r\n", (*timIt)->GetTimerID(), (*timIt)->GetDuration());

					// build and queue timer command
					wchar_t *cmd = new wchar_t[MAX_CMD];
					wchar_t *p = cmd;
					p = wcsapp(p, L"opcmsg a=UGMon o=Heart-beat -node=");
					p = wcsapp(p, (*timIt)->GetTimerID());

					//wcscat(cmd, (*timIt)->GetTimerID());
					if((*timIt)->GetTimerType() == heartBeatExpired)
					{
						p = wcsapp(p, L" msg_t=\"No heart-beat received from node: ");
						p = wcsapp(p, (*timIt)->GetTimerID());
					}
					else if((*timIt)->GetTimerType() == maintExpired)
					{
						p = wcsapp(p, L" msg_t=\"Maintenance has ended for node: ");
						p = wcsapp(p, (*timIt)->GetTimerID());
					}
					p = wcsapp(p, L"\" -option hostname=");
					p = wcsapp(p, (*timIt)->GetTimerID());

					dd(Facility, L"Queued command: %s\r\n", cmd);
					ExecObj *newCmd = new ExecObj();
					newCmd->setCmdStr(cmd);
					delete [] cmd;
					CmdQueue.push(newCmd);

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
	
	trapReceiver->Stop();
	
	// Stop SNMP trap engine, and empty queue
	dd(Facility, L"Stopping SNMP trap receiver, received %u trap(s)\r\n", trapReceiver->GetTrapCount());
	unsigned int trapPopCount = 0;

	SSnmpPacket *newPacket;
	while (pQueue.try_pop(newPacket))
	{
		delete newPacket;
		trapPopCount++;
	}
	delete trapReceiver;

	if(trapPopCount > 0)
		dw(Facility, L"Due to application shutdown, %u SNMP trap(s) were dropped\r\n", trapPopCount);

    // TODO save timers and host state so that we can reload them from database
	// Save any running timers, and delete the timer list
	dd(Facility, L"%u timer(s) active upon application shutdown, saving state...\r\n", ActiveTimers.size());
	std::list<TimerObj*>::iterator timIt = ActiveTimers.begin();
	while (timIt != ActiveTimers.end())
	{
		delete (*timIt);
		timIt = ActiveTimers.erase(timIt);
	}

	// Delete all FileSystems
	while(!FileSystems.empty())
	{
		delete FileSystems.back();
		FileSystems.pop_back();
	}

	// Delete all Thresholds
	while(!Thresholds.empty())
	{
		delete Thresholds.back();
		Thresholds.pop_back();
	}

    // Empty host list
    FreeHosts();

	print(Facility, L"Executed %u command(s)\r\n", cmdQueue.GetExecutedCommands());

	print(Facility, L"Main loop terminated\r\n");
	return 0;
}

// Program configuration reader
bool ReadConf()
{
	// Read the connection string from registry
	HKEY hKey;
	if(RegOpenKey(HKEY_LOCAL_MACHINE, wcsTrapHandlerRegKey, &hKey) == ERROR_SUCCESS)
	{
		if(wcsInstallPath != NULL)
			delete [] wcsInstallPath;

		wcsInstallPath = new wchar_t [MAX_PATH+1];
		DWORD dwSize = MAX_PATH*sizeof(wchar_t);
		DWORD regType;

		if(RegQueryValueEx(hKey, L"Install Path", 0, &regType, (BYTE*)wcsInstallPath, &dwSize) != ERROR_SUCCESS)
		{
			wprintf(L"unable to read install path\r\n");
			RegCloseKey(hKey);
			return false;
		}

		if(wcsConnectionString != NULL)
			delete [] wcsConnectionString;

		wcsConnectionString = new wchar_t [65536];
		dwSize = 65535*sizeof(wchar_t);
		if(RegQueryValueEx(hKey, L"Connection String", 0, &regType, (BYTE*) wcsConnectionString, &dwSize) != ERROR_SUCCESS)
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

// Configuration builder
bool CreateConf(const wchar_t *installPath, const wchar_t *instanceName, const wchar_t *dbName)
{
	HKEY hKey;
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, wcsTrapHandlerRegKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS)
	{
		if(RegSetValueEx(hKey, L"Install Path", 0, REG_SZ, (BYTE*)installPath, (DWORD)(wcslen(installPath)+1)*sizeof(wchar_t)) != ERROR_SUCCESS)
		{
			RegCloseKey(hKey);
			// wprintf(L"Unable to set install path\r\n");
			return false;
		}

		// build the connection string

		if(wcsConnectionString != NULL)
			delete [] wcsConnectionString;

		wcsConnectionString = new wchar_t[65536];

		wcscpy(wcsConnectionString, L"Provider=SQLOLEDB.1;Integrated Security=SSPI;Persist Security Info=False;Initial Catalog=");
		wcscat(wcsConnectionString, dbName);
		wcscat(wcsConnectionString, L";Data Source=");
		wcscat(wcsConnectionString, instanceName);
		wcscat(wcsConnectionString, L";Use Procedure for Prepare=1;Auto Translate=True;Packet Size=4096;Workstation ID=");

		wcscat(wcsConnectionString, wcsLocalHostName);
		wcscat(wcsConnectionString, L";Use Encryption for Data=False;Tag with column collation when possible=False");
		

		if(RegSetValueEx(hKey, L"Connection String", 0, REG_SZ, (BYTE*)wcsConnectionString, (DWORD)(wcslen(wcsConnectionString)+1)*sizeof(wchar_t)) != ERROR_SUCCESS)
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

bool ServiceDelete()
{
	PrintError(IDS_DELETESERVICEMSG);
	
	if(! Service::Delete(wcsServiceName))
	{
		wprintf(L"Service failed to uninstall\r\n");
		return false;
	}
	wprintf(L"Service successfully uninstalled\r\n");
	return true;
}

bool ServiceInstall(const wchar_t *pathName)
{
	size_t dirLength = wcslen(pathName);
	PrintError(IDS_INSTALLMSG);
	// construct the binary path name
	wchar_t binName[MAX_CMD];
	wcscpy(binName, pathName);
	if(binName[wcslen(binName)-1] != L'\\')
		wcscat(binName, L"\\");

	wcscat(binName, wcsExeFileName);

	// Check wether the file exist
	HANDLE hFile = CreateFile(binName, GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		if(err == 0x02 || err == 0x03)	// File not found
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
	if(!Service::Install(wcsServiceName, wcsServiceLabel, binName))
	{
		PrintError(IDS_ERROR_SERVICEINST);
		return false;
	}

	// Service should now be installed, lets configure the registry DB correctly
	wchar_t RegKeyPath[MAX_PATH+1];
	wcscpy(RegKeyPath, L"SYSTEM\\CurrentControlSet\\Services\\");
	wcscat(RegKeyPath, wcsServiceName);
	// wprintf(L"RegDB: %s\n", RegKeyPath);
	HKEY hKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, RegKeyPath, 0, KEY_WRITE | KEY_READ, &hKey) != ERROR_SUCCESS)
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

	if(RegSetValueEx(hKey, L"Description", 0, REG_SZ, (BYTE*)&wcsServiceDescr, (DWORD)(wcslen(wcsServiceDescr)+1)*sizeof(wchar_t)) != ERROR_SUCCESS)
	{
		wprintf(L"Unable to set service description\r\n");
		RegCloseKey(hKey);
		ServiceDelete();
		return false;
	}

	RegCloseKey(hKey);
	
	return true;
}

inline TimerObj* LocateTimer(const wchar_t *id, const timerType type)
{
	std::list<TimerObj*>::iterator it;
	for (it = ActiveTimers.begin(); it != ActiveTimers.end(); it++)
	{
		if ((*it)->GetTimerType() == type && wcsicmp((*it)->GetTimerID(), id) == 0)
			return (*it);
	}
	return NULL;
}

inline void DeleteTimer(const wchar_t *id, const timerType type)
{
	auto it = ActiveTimers.begin();
	while (it != ActiveTimers.end())
	{
		if ((*it)->GetTimerType() == type && wcsicmp((*it)->GetTimerID(), id) == 0)
		{
			delete *it;
			it = ActiveTimers.erase(it);
			return;
		}
		it++;
	}
}

#pragma warning (disable: 4312)

#endif