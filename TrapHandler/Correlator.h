#ifndef __CORRELATOR_HEAD
#define __CORRELATOR_HEAD

#include "stdafx.h"
#include <aclapi.h>
#include "service.h"
#include "correlation.h"
// #include "AtomicQueue.h"
#include "Queue.h"
#include <list>
#include "ExecObj.h"

#pragma comment (lib, "Version.lib") // to read the VS_VERSION_INFO structure
#pragma warning (disable :4996)

////////////////////////////////
// Globals

HINSTANCE hInst = 0;
wchar_t wcsExeFileName[256];
wchar_t wcsServiceName[256];
wchar_t wcsServiceLabel[256];
wchar_t wcsServiceDescr[2049];

////////////////////////////////
DWORD WINAPI queueFunc(LPVOID p);

void LoadGlobalStrings()
{
	hInst = GetModuleHandle(NULL);
	LoadString(hInst, IDS_EXEFILENAME, wcsExeFileName, 255);
	LoadString(hInst, IDS_SERVICESHORTNAME, wcsServiceName, 255);
	LoadString(hInst, IDS_SERVICENAME, wcsServiceLabel, 255);
	LoadString(hInst, IDS_SERVICEDESCR, wcsServiceDescr, 2048);
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

// if we find a correlation with the same id as pCorr, pop it from the list
void RemoveFromList(std::list<Correlation*> *l, Correlation *pCorr)
{
	std::list<Correlation*>::iterator it;
	for(it = l->begin(); it != l->end(); it++)
	{
		if(wcscmp((*it)->generalInfo.id, pCorr->generalInfo.id) == 0)
		{
			delete (*it);
			it = l->erase(it);
			return;
		}
	}
}

// given a correlation pointer and list pointer, this function retrieves the iterator
// corresponding to the existing iterator in the list with the same id
// if the id cannot be found, list.end() is returned
std::list<Correlation*>::iterator GetIteratorFromList(std::list<Correlation*> *l, Correlation *pCorr)
{
	std::list<Correlation*>::iterator it;
	for(it = l->begin(); it != l->end(); it++)
	{
		if(wcscmp((*it)->generalInfo.id, pCorr->generalInfo.id) == 0)
			return it;
	}
	return l->end();
}

// Main program loop
DWORD CorrelatorMainFn(LPDWORD pParam)
{
	///////////////////
	// AtomicQueue<Correlation*> queue(MAXQSIZE);	// initialize the request queue
	SimpleQueue queue(MAXQSIZE);	// initialize the request queue
	std::list<Correlation*> Timers;				// the list of active timers
	std::list<ExecObj*> TimerCommands;			// the list of currently running commands
	DWORD dwTID;	// used to store the thread ID
	DWORD dwCurrentCommands = 0;
	DWORD dwMaxCommands = MAXCOMMANDS;

	unsigned int eventCount = 0, commandBeginCount=0, commandEndCount=0;

	// QueueFunc will receive all event creations/deletions
	// we read the queue to create, update and kill timers
	HANDLE hThread = CreateThread(NULL, 0, queueFunc, &queue, 0, &dwTID);

	DWORD dwWaitTime = 100;

	DWORD dwWaitResult = 0;
	BOOL bQuit = false;
	
	while(!bQuit)
	{
		dwWaitResult = WaitForSingleObject(hQuitEvent, dwWaitTime);

		// Do we have to quit?
		if((!bDebug && dwWaitResult == WAIT_OBJECT_0) || (bDebug && _kbhit() != 0))
		{
			if(bDebug)
				_getwch();
			break;
		}
		else
		{
			// process request queue
			Correlation *pCorr = NULL;
			while(queue.Pop((void*&)pCorr))
			{
				if(pCorr->generalInfo.type == CORR_DELETE)
				{	// Remove the correlator from the timer list
					RemoveFromList(&Timers, pCorr);
					eventCount++;
					delete pCorr;
				}
				else
				{
					std::list<Correlation*>::iterator it = GetIteratorFromList(&Timers, pCorr);
					eventCount++;
					if(it == Timers.end())	// we have to create a new correlation
						Timers.push_back(pCorr);
					else
					{
						(*it)->generalInfo.duration = pCorr->generalInfo.duration;
						memcpy(&(*it)->generalInfo.startTime, &pCorr->generalInfo.startTime, sizeof(FILETIME));
						(*it)->generalInfo.nTriggers++;
						eventCount++;
						delete pCorr;
					}
					
				}
			}	// while queue.Pop

			// Check for any completed timer commands
			std::list<ExecObj *>::iterator cit;
			for(cit=TimerCommands.begin(); cit != TimerCommands.end(); cit++)
			{
				if((*cit)->GetCmdState() != 0)	// command has completed
				{
					delete (*cit);
					cit = TimerCommands.erase(cit);
					commandEndCount++;
					dwCurrentCommands--;
					if(cit == TimerCommands.end())
						break;
					
				}
			}

			// update all timers
			
			FILETIME ft;
			GetSystemTimeAsFileTime(&ft);

			std::list<Correlation *>::iterator it;
			for(it = Timers.begin(); it != Timers.end(); it++)
			{
				if(dwCurrentCommands >= dwMaxCommands)
					break;	// we have no command slots to process actions
				
				if((*it)->IsExpired(&ft))
				{
					// Execute associated command
					if((*it)->generalInfo.actionStr != NULL)
					{
						ExecObj *cmd = new ExecObj();
						
						cmd->RunCmd((*it)->generalInfo.actionStr);
						TimerCommands.push_back(cmd);
						dwCurrentCommands++;
						commandBeginCount++;
						print(L"id: %s expired, %u commands active, running command %s\n", (*it)->generalInfo.id, dwCurrentCommands, (*it)->generalInfo.actionStr);
					}
					delete (*it);
					it = Timers.erase(it);
					if(it == Timers.end())
						break;
				}
			}

		}
	}

	print(L"Shutting down event listener, received %u events, and started %u commands (%u completed), \r\n", eventCount, commandBeginCount, commandEndCount);
	bDoShutdown = true;
	WaitForSingleObject(hThread, 10000);	// give the listener a chance to shut down
	CloseHandle(hThread);

	// clear request queue
	Correlation *pTmp = NULL;
	while(queue.Pop((void*&)pTmp))
		delete pTmp;

	// clear timer queue
	while(!Timers.empty())
	{
		print(L"Shutting down timer ID %s\r\n", Timers.front()->generalInfo.id);
		delete Timers.front();
		Timers.pop_front();
	}

	// clear running command queue
	// Check for any completed timer commands
	std::list<ExecObj *>::iterator cit;
	for(cit=TimerCommands.begin(); cit != TimerCommands.end(); cit++)
	{
		if((*cit)->GetCmdState() != 0)	// command has completed
		{
			delete (*cit);
			cit = TimerCommands.erase(cit);
			dwCurrentCommands--;
			commandEndCount++;
			if(cit == TimerCommands.end())
				break;
		}
		else
		{
			(*cit)->GetCmdReturn(2000);
			dwCurrentCommands--;
			commandEndCount++;
			delete (*cit);
			cit = TimerCommands.erase(cit);
			if(cit == TimerCommands.end())
				break;
		}
	}

	print(L"Main loop terminated\r\n");
	return 0;
}

// Program configuration reader
bool ReadConf()
{
	return true;
}

// Configuration builder
bool CreateConf(const wchar_t *installPath)
{
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
	wchar_t binName[MAX_PATH+1];
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
	wprintf(L"RegDB: %s\n", RegKeyPath);
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

#pragma warning (disable: 4312)

////////////////////////////
// Message receiver
DWORD WINAPI queueFunc(LPVOID p)
{
	DWORD dwMaxMsgSize = MAXMSGSIZE;
#pragma region setSecDescr
		// All this shit is simply to allow everyone in the administrators group access to the objects
		DWORD dwRes;
		PSID pEveryoneSID = NULL;
		PACL pACL = NULL;
		PSECURITY_DESCRIPTOR pSD = NULL;
		EXPLICIT_ACCESS_W ea;
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		SECURITY_ATTRIBUTES sa;

		// Create a well-known SID for the Everyone group.
		if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
						 SECURITY_WORLD_RID,
						 0, 0, 0, 0, 0, 0, 0,
						 &pEveryoneSID))
		{
			print(L"AllocateAndInitializeSid Error %u\r\n", GetLastError());
			goto Cleanup;
		}

		// Initialize an EXPLICIT_ACCESS structure for an ACE.
		// The ACE will allow Everyone read access to the key.
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS_W));
		ea.grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance= NO_INHERITANCE;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

		// Create a new ACL that contains the new ACEs.
		dwRes = SetEntriesInAclW(1, &ea, NULL, &pACL);
		if (ERROR_SUCCESS != dwRes) 
		{
			print(L"SetEntriesInAcl Error %u\r\n", GetLastError());
			goto Cleanup;
		}

		// Initialize a security descriptor.  
		pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
								 SECURITY_DESCRIPTOR_MIN_LENGTH); 
		if (NULL == pSD) 
		{ 
			print(L"LocalAlloc Error %u\r\n", GetLastError());
			goto Cleanup; 
		} 
	 
		if (!InitializeSecurityDescriptor(pSD,
				SECURITY_DESCRIPTOR_REVISION)) 
		{  
			print(L"InitializeSecurityDescriptor Error %u\r\n", GetLastError());
			goto Cleanup; 
		} 
	 
		// Add the ACL to the security descriptor. 
		if (!SetSecurityDescriptorDacl(pSD, 
				TRUE,     // bDaclPresent flag   
				pACL, 
				FALSE))   // not a default DACL 
		{  
			print(L"SetSecurityDescriptorDacl Error %u\r\n", GetLastError());
			goto Cleanup; 
		} 

		// Initialize a security attributes structure.
		sa.nLength = sizeof (SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = TRUE;

		// end of shit
#pragma endregion

	// AtomicQueue<Correlation*> *queue = reinterpret_cast <AtomicQueue<Correlation*>*> (p);
	SimpleQueue *queue = reinterpret_cast <SimpleQueue*> (p);

	HANDLE msgIncomming = CreateEvent(&sa,true,false,L"Global\\UCorrelatorEvent");
	HANDLE msgBufLocked = CreateMutex(&sa,false,L"Global\\UCorrelatorMutex");
	HANDLE hMap = CreateFileMapping(reinterpret_cast<HANDLE>(0xFFFFFFFF),&sa,PAGE_READWRITE,0,dwMaxMsgSize,L"Global\\UCorrelator");
	if(!hMap)
		return 1;

	unsigned char *pSharedCorrInfo = static_cast<unsigned char*>(MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS,0,0,0));
	ZeroMemory(pSharedCorrInfo, sizeof(SCorrelator));	// reset shared memory with zeroes

	print(L"Queue Started\r\n");

	while(!bDoShutdown)
	{
		// use wait for single object - is we can flag a global event from TTForwarder
		if(WaitForSingleObject(msgIncomming, 500) == WAIT_OBJECT_0) // check every half second, if have to shutdown
		{
			if(WaitForSingleObject(msgBufLocked, 5000) != WAIT_OBJECT_0)
			{
				print(L"Timeout waiting for buffer mutex to free!\r\n");
				return 2;
			}
			
			DWORD action;
			memcpy(&action, pSharedCorrInfo, sizeof(DWORD));
			Correlation *newevent = new Correlation(pSharedCorrInfo+sizeof(DWORD));

			if(action == 1)	// delete timer
				newevent->generalInfo.type = CORR_DELETE;
			
			ResetEvent(msgIncomming);	// we are ready to receive a new event
			ReleaseMutex(msgBufLocked);

			queue->Push(newevent);		// push the received event

				if (action == 0)
					{print(L"Timer creation queued for id: %s\r\n", newevent->generalInfo.id);}
				else
					{print(L"Deleting timer %s\n", newevent->generalInfo.id);}
		}
	}

	CloseHandle(msgIncomming);
	CloseHandle(msgBufLocked);
	CloseHandle(hMap);

	Cleanup:

    if (pEveryoneSID) 
        FreeSid(pEveryoneSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);

	print(L"Listener shut down\r\n");
	return 0;
}

#endif