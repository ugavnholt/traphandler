#include "stdafx.h"
#include "DBUtil.h"
#include "HostInfo.h"
#include <list>

using namespace std;

list<HostInfo*> hosts;

HostInfo* GetHostByName(const wchar_t *hostName)
{
    list<HostInfo*>::iterator it = hosts.begin();
    while(it != hosts.end())
    {
        if(wcscmp((*it)->LocalHostName, hostName) == 0)
            return (*it);
        it++;
    }
    return NULL;
}

HostInfo* GetHostByIP(const wchar_t *hostIP)
{
    list<HostInfo*>::iterator it = hosts.begin();
    while(it != hosts.end())
    {
        if(wcscmp((*it)->SourceIP, hostIP) == 0)
            return (*it);
        it++;
    }
    return NULL;
}

HostInfo* GetHostByID(const GUID &ID)
{
    list<HostInfo*>::iterator it = hosts.begin();
    while(it != hosts.end())
    {
        if(memcmp(&(*it)->ID, &ID, sizeof(GUID)) == 0)
            return (*it);
        it++;
    }
    return NULL;
}

// Loads all known hosts from database
void LoadHosts(CSession *dbSession)
{
    //HostInfo(const wchar_t *HostName, const wchar_t *IPAddress, const GUID *ID, int LastStatus = STATUS_UNKNOWN)
    const wchar_t *sqlCmd = L"SELECT ID, LocalHostName, LastSeenIP, LastKnownStatus from Hosts";
     CCommand<CDynamicAccessor, CRowset> *cmd = ExecuteDbSelectCmd(dbSession, sqlCmd);

     while(cmd->MoveNext() == S_OK)
     {
         hosts.push_front(new HostInfo(
             (wchar_t*)cmd->GetValue(L"LocalHostName"),
             (wchar_t*)cmd->GetValue(L"LastSeenIP"),
             (GUID*)cmd->GetValue(L"ID"),
             *((int*)cmd->GetValue(L"LastKnownStatus"))));
         dd(L"DB     ", L"Loaded host: %s, sourceIP: %s from database\r\n", hosts.front()->LocalHostName, hosts.front()->SourceIP);
     }

     dn(L"DB     ", L"Loaded %u host(s) from database\r\n", hosts.size());
	 cmd->Close();
	 delete cmd;
}

void FreeHosts()
{
    while(!hosts.empty())
    {
        delete hosts.front();
        hosts.pop_front();
    }
}

#ifndef __HOSTCONTROL_INCLUDE
#define __HOSTCONTROL_INCLUDE

void UpdateHost(
    CSession *dbSession,
    const wchar_t *hostName, 
    const wchar_t *platform,
    const wchar_t *versionStr,
    const wchar_t *UgmonVer,
    const wchar_t *SourceIP,
    const wchar_t *proxyHostName)
{
    DWORD dwMajorVer, dwMinorVer, dwSPLevel;
    wchar_t *Architecture = NULL, *Build = NULL;
    const wchar_t *pVer = versionStr;
    // Extract what we can from the versionStr
    pVer = ParseVersionString(versionStr, &dwMajorVer, &dwMinorVer, Build, &dwSPLevel, Architecture);

    if(pVer != NULL)
    {
        //wprintf(L"VersionString: %s extracts to:\n\tMajorVersion: %u\n\tMinorVersion: %u\n\tBuild: %s\n\tSP Level: %u\n\tArch: %s\n\tDescr: %s\n",
        //    versionStr, dwMajorVer, dwMinorVer, Build, dwSPLevel, Architecture, pVer);
    }
    else
        pVer = versionStr;

    // Lets check if the host should be updated or created
    wchar_t *sqlCmd = new wchar_t[2048];
    wchar_t *p = sqlCmd;

    p = wcsapp(sqlCmd, L"SELECT ID FROM dbo.Hosts WHERE LOWER(LocalHostName) = LOWER('");

    p = wcsapp(p, hostName);
    p = wcsapp(p, L"')");
    CCommand<CDynamicAccessor, CRowset> *cmd = ExecuteDbSelectCmd(dbSession, sqlCmd);
    if(cmd->MoveNext() != S_OK)
    {
        cmd->Close();
        delete cmd;

        wchar_t *tmpStr = new wchar_t[10];
        p = wcsapp(sqlCmd, 
L"INSERT INTO dbo.hosts \
(ID, LocalHostName, LastSeenIP, UGMonVersion, VersionString, OSVersionMajor, OSVersionMinor, OSVersionBuild, \
ServicePack, Architecture, ProxyHostName, Platform) VALUES (NEWID(),LOWER('");
        p = wcsapp(p, hostName);
        p = wcsapp(p, L"'),'");
        p = wcsapp(p, SourceIP);
        p = wcsapp(p, L"','");
        p = wcsapp(p, UgmonVer);
        p = wcsapp(p, L"','");
        p = wcsapp(p, pVer);
        p = wcsapp(p, L"','");
        _itow(dwMajorVer, tmpStr, 10);
        p = wcsapp(p, tmpStr); //dwMajorVer
        p = wcsapp(p, L"','");
        _itow(dwMinorVer, tmpStr, 10);
        p = wcsapp(p, tmpStr); // dwMinorVer
        p = wcsapp(p, L"','");
        p = wcsapp(p, Build);
        p = wcsapp(p, L"','");
        _itow(dwSPLevel, tmpStr, 10);
        p = wcsapp(p, tmpStr); //dwSPLevel
        p = wcsapp(p, L"','");
        p = wcsapp(p, Architecture);
        p = wcsapp(p, L"','");
        p = wcsapp(p, proxyHostName);
        p = wcsapp(p, L"','");
        p = wcsapp(p, platform);
        p = wcsapp(p, L"')");
        // wprintf(L"%s\n", sqlCmd);

        delete [] tmpStr;

        HRESULT hr = ExecuteDbCmd(dbSession, sqlCmd);
        if(FAILED(hr))
        {
            dmi(L"DB    ", L"Unable to store information about %s in host table, error code: 0x%x\r\n", hostName, hr);
            dd(L"DB    ", L"Failing query was %s\r\n", sqlCmd);
        }
    }
    else    // Host exists, we need to update it
    {
        GUID ID;
        memcpy(&ID, cmd->GetValue(L"ID"), sizeof(GUID));
        cmd->Close();
        delete cmd;

        wchar_t *tmpStr = new wchar_t[40];
        
        p = wcsapp(sqlCmd, 
L"UPDATE dbo.hosts SET LastSeenIP='");
        p = wcsapp(p, SourceIP);
        p = wcsapp(p, L"',UGMonVersion='");
        p = wcsapp(p, UgmonVer);
        p = wcsapp(p, L"',versionString='");
        p = wcsapp(p, pVer);
        p = wcsapp(p, L"',OSVersionMajor='");
        _itow(dwMajorVer, tmpStr, 10);
        p = wcsapp(p, tmpStr); //dwMajorVer
        p = wcsapp(p, L"',OSVersionMinor='");
        _itow(dwMinorVer, tmpStr, 10);
        p = wcsapp(p, tmpStr); // dwMinorVer
        p = wcsapp(p, L"',OSVersionBuild='");
        p = wcsapp(p, Build);
        p = wcsapp(p, L"',ServicePack='");
        _itow(dwSPLevel, tmpStr, 10);
        p = wcsapp(p, tmpStr); //dwSPLevel
        p = wcsapp(p, L"',Architecture='");
        p = wcsapp(p, Architecture);
        p = wcsapp(p, L"',ProxyHostName='");
        p = wcsapp(p, proxyHostName);
        p = wcsapp(p, L"',Platform='");
        p = wcsapp(p, platform);
        p = wcsapp(p, L"', LastHelloTime=GETDATE() WHERE ID='");
        swprintf(tmpStr, 39, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", ID.Data1, ID.Data2, ID.Data3,
            ID.Data4[0], ID.Data4[1], ID.Data4[2], ID.Data4[3], ID.Data4[4], ID.Data4[5], ID.Data4[6], ID.Data4[7]);
        p = wcsapp(p, tmpStr);
        p = wcsapp(p, L"'");
        // wprintf(L"%s\n", sqlCmd);

        delete [] tmpStr;

        HRESULT hr = ExecuteDbCmd(dbSession, sqlCmd);
        if(FAILED(hr))
        {
            dmi(L"DB    ", L"Unable to update information about %s in host table, error code: 0x%x\r\n", hostName, hr);
            dd(L"DB    ", L"Failing query was %s\r\n", sqlCmd);
            //dd(L"DB    ", L"Failed query was: %s\r\n", sqlCmd);
        }

    }

    if(Architecture != NULL) delete [] Architecture;
    if(Build != NULL) delete [] Build;

    delete [] sqlCmd;
}



#endif