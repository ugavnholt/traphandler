#ifndef __HOSTINFO_HEADER
#define __HOSTINFO_HEADER

#include "stdafx.h"
#include "DBUtil.h"

// Status values for host binary flag
// bit0 - set - ping is available, unset - host can't be pinged
// bit1 - set - responding to ping, unset - not responding to ping
// bit2 - set - Heart beat received, unset - no heartbeat received
// bit3 - set - UGMon running - unset UGMon not running
#define STATUS_UNKNOWN          0   // No status determined for host
#define STATUS_DOPING           1   // Status has not yet been determined
#define STATUS_PINKOK           2   // Ping is working and responding
#define STAUTS_HBOK             4   // Ping never worked for sourceIP
#define STATUS_UGMONRUNNING     8   // Ping has worked, but not anymore

// Given a version string, individual contained elements are extracted and returned
// return pointer to the start of the non-parsed string if the version string was known, and the output variables are set
// otherwise returns NULL
// Allocates memory for Build, and Architecture which must be freed by client
wchar_t  *ParseVersionString(const wchar_t *VersionString, 
                    DWORD *VerMajor,
                    DWORD *VerMinor,
                    wchar_t *&Build,
                    DWORD *SPLevel,
                    wchar_t *&Architecture)
{
    wchar_t *pEndStr = NULL;
    size_t strLen;
    if(VersionString == NULL || Architecture != NULL || Build != NULL)
        goto ErrorExit;

#pragma region WinParser
    if(wcsstr(VersionString, L"MSWin:") == VersionString)
    {   // Windows formatted version string
        // Example: MSWin:5.2,BUILD:3790,Service Pack 2-x32     Microsoft Windows Server 2003 R2, Enterprise Edition
        
        wchar_t *pStr = (wchar_t *)VersionString;
        pStr += 6;  // point to the major version

        *VerMajor = wcstoul(pStr, &pStr, 10);
        if(wcsstr(pStr, L".") != pStr)
            goto ErrorExit;
        pStr++; // Skip the "."

        *VerMinor = wcstoul(pStr, &pStr, 10);
        if(wcsstr(pStr, L",BUILD:") != pStr)
            goto ErrorExit;
        pStr+=7; // Skip the ",BUILD:"
        wcstoul(pStr, &pEndStr, 10);

        strLen = pEndStr-pStr;
        Build = new wchar_t[strLen+1];
        wcsncpy(Build, pStr, strLen);
        Build[strLen] = L'\0';

        pStr = pEndStr;

        if(wcsstr(pStr, L",Service Pack ") != pStr)
        {
            if(wcsstr(pStr, L",No Service Pack") == pStr)
            {
                *SPLevel = 0;
                pStr+=17; // skip until arch field
            }
            else
                goto ErrorExit;
        }
        else
        {
            pStr+=14;   // skip the ",Service Pack "
            *SPLevel = wcstoul(pStr, &pStr, 10);
            if(wcsstr(pStr, L"-") == NULL)
                goto ErrorExit;
            pStr++;     // skip the "-"
        }

        // Architecture
        pEndStr = pStr;
        while(*pEndStr != L' ' && *pEndStr != L'\t' && *pEndStr != L'\0')
            pEndStr++;
        if(*pStr == L'\0')
            goto ErrorExit;

        strLen = pEndStr-pStr;
        Architecture = new wchar_t[strLen+1];
        wcsncpy(Architecture, pStr, strLen);
        Architecture[strLen] = L'\0';

         while((*pEndStr == L' ' || *pEndStr == L'\t') && *pEndStr != L'\0')
            pEndStr++;
        // pEndStr points to the beginning of the OS description string
    }
#pragma endregion
#pragma region LinuxParser
    else if(wcsstr(VersionString, L"Linux ") == VersionString)
    {   // Linux formatted version string
        // example: Linux osi1155 2.6.31-14-generic-pae #48-Ubuntu SMP Fri Oct 16 15:22:42 UTC 2009 i686 GNU/Linux
        // example: Linux osi0518.de-prod.dk 2.6.18-164.el5 #1 SMP Tue Aug 18 15:51:48 EDT 2009 x86_64 x86_64 x86_64 GNU/Linux

        wchar_t *pStr = (wchar_t *)VersionString;

        // Skip "Linux "
        pStr+=6;

        // Skip "<Hostname> "
        while (*pStr != L' ')
            pStr++;
        pStr++;

        *VerMajor = wcstoul(pStr, &pStr, 10);
        if(*pStr != L'.')
            goto ErrorExit;
        pStr++;

        *VerMinor = wcstoul(pStr, &pStr, 10);

        if(*pStr != L'.')
            goto ErrorExit;
        pStr++;

        // Rest of the versiontext is the build
        pEndStr = pStr;
        while(*pEndStr != L' ' && *pEndStr != L'\t' && *pEndStr != L'\0')
            pEndStr++;
        if(*pStr == L'\0')
            goto ErrorExit;

        strLen = pEndStr-pStr;
        Build = new wchar_t[strLen+1];
        wcsncpy(Build, pStr, strLen);
        Build[strLen] = L'\0';

        pStr = pEndStr+1;
        if(*pStr != L'#')
            goto ErrorExit;

        pEndStr = wcsstr(pStr, L" GNU/Linux");
        pStr = pEndStr-1;

        while(pStr >= VersionString && *pStr != L' ')
            pStr--;
        if(pStr == VersionString)
            goto ErrorExit;

        strLen = pEndStr-pStr;
        Architecture = new wchar_t[strLen+1];
        wcsncpy(Architecture, pStr, strLen);
        Architecture[strLen] = L'\0';

        *SPLevel = 0;
        pEndStr = (wchar_t *)VersionString;
    }
#pragma endregion
#pragma region hpux_parser
    else if(wcsstr(VersionString, L"HP-UX ") == VersionString)
    {   // HPUX Example:
        // HP-UX ux2b B.11.11 U 9000/800 2056945439 unlimited-user license

         wchar_t *pStr = (wchar_t *)VersionString;

        // Skip "HP-UX "
        pStr+=6;

        // Skip "<Hostname> "
        while (*pStr != L' ')
            pStr++;
        pStr++;

        pStr+=2; // Skip "B."

        *VerMajor = wcstoul(pStr, &pStr, 10);
        if(*pStr != L'.')
            goto ErrorExit;
        pStr++;

        *VerMinor = wcstoul(pStr, &pStr, 10);
        if(*pStr != L' ')
            goto ErrorExit;
        pStr+=3;

        // Architecture is the next
        pEndStr = pStr;
        while (*pEndStr != L' ' && *pEndStr != L'\0')
            pEndStr++;
        strLen = pEndStr-pStr;
        Architecture = new wchar_t[strLen+1];
        wcsncpy(Architecture, pStr, strLen);
        Architecture[strLen] = L'\0';
        
        if(Build != NULL) delete [] Build;
        Build = new wchar_t[1];
        Build[0] = L'\0';
        *SPLevel = 0;
        pEndStr=(wchar_t *)VersionString;
    }
#pragma endregion
    else
        goto ErrorExit;

    return pEndStr;

ErrorExit:
    dw(L"UPARS ", L"Unable to parse version string \"%s\"\r\n", VersionString);
    if(Build != NULL) delete [] Build;
    Build = new wchar_t[1];
    Build[0] = L'\0';
    if(Architecture != NULL) delete [] Architecture;
    Architecture = new wchar_t[1];
    Architecture[0] = L'\0';
    *VerMajor = 0;
    *VerMinor = 0;
    *SPLevel = 0;
    return NULL;
}

class HostInfo
{
public:
    wchar_t *LocalHostName;
    wchar_t *SourceIP;
    GUID    ID;
    int     Status;

    HostInfo(const wchar_t *HostName, const wchar_t *IPAddress, const GUID *ID, int LastStatus = STATUS_UNKNOWN)
    {
        size_t strLen = wcslen (HostName);
        if(strLen > 0)
        {
            LocalHostName = new wchar_t[strLen+1];
            wcscpy(LocalHostName, HostName);
        }
        else
        {
            LocalHostName = NULL;
            dma(L"HOSTINF", L"received NULL hostname\r\n"); 
        }
        
        strLen = wcslen(IPAddress);
        if(strLen > 0)
        {
            SourceIP = new wchar_t[strLen+1];
            wcscpy(SourceIP, IPAddress);
        }
        else
        {
            LocalHostName = NULL;
            dma(L"HOSTINF", L"received NULL address\r\n");
        }

        memcpy(&ID, ID, sizeof(GUID));

        Status = LastStatus;
    }

    ~HostInfo()
    {
        if(LocalHostName != NULL) delete [] LocalHostName;
        if(SourceIP != NULL) delete [] SourceIP;
    }
};


#endif