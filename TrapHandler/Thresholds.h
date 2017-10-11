#ifndef __THRESHOLDS_HEAD
#define __THRESHOLDS_HEAD

#include "FSThresholds.h"
#include "strmatch.h"

// Forward declarations
class CThreshold;
class CFS;
CThreshold *GetThresholdForFS(const wchar_t *hostName, const wchar_t *volumeName, ULONGLONG fsSizeMB);

// Global declarations
std::vector<CThreshold*> Thresholds;
std::vector<CFS*> FileSystems;

const wchar_t *ThreshTypes[] = {L"Normal", L"Warning", L"High"};

class CThreshold
{
public:
	wchar_t *hostExpr;
	wchar_t *volExpr;

	ULONGLONG HighFreeMegsThresh;
	ULONGLONG WarnFreeMegsThresh;

	double	HighUtilThresh;
	double	WarnUtilThresh;

	wchar_t *WarnSevStr;
	wchar_t *HighSevStr;

	ULONGLONG minFSSize, maxFSSize;

	CThreshold(CFSThresholds *newThresh) : hostExpr(NULL), volExpr(NULL), WarnSevStr(NULL), HighSevStr(NULL), minFSSize(0), maxFSSize(0)
	{
		WarnSevStr = new wchar_t[newThresh->m_dwWarnSevLength/sizeof(wchar_t)+1];
		memcpy(WarnSevStr, &newThresh->m_WarnSev, newThresh->m_dwWarnSevLength+sizeof(wchar_t));

		HighSevStr = new wchar_t[newThresh->m_dwhighSevLength/sizeof(wchar_t)+1];
		memcpy(HighSevStr, &newThresh->m_highSev, newThresh->m_dwhighSevLength+sizeof(wchar_t));

		hostExpr = new wchar_t[newThresh->m_dwHostExpressionLength/sizeof(wchar_t)+1];
		memcpy(hostExpr, newThresh->m_HostExpression, newThresh->m_dwHostExpressionLength+sizeof(wchar_t));

		volExpr = new wchar_t[newThresh->m_dwVolumeExpressionLength/sizeof(wchar_t)+1];
		memcpy(volExpr, newThresh->m_VolumeExpression, newThresh->m_dwVolumeExpressionLength+sizeof(wchar_t));

		HighFreeMegsThresh = newThresh->m_ThreshFreeMBHigh;
		WarnFreeMegsThresh = newThresh->m_TreshFreeMBWarn;

		HighUtilThresh = newThresh->m_ThreshUtilHigh;
		WarnUtilThresh = newThresh->m_ThreshUtilWarn;

		minFSSize = newThresh->m_MinFSSizeMB;
		maxFSSize = newThresh->m_MaxFSSizeMB;
	}

	~CThreshold()
	{
		if(WarnSevStr != NULL) delete [] WarnSevStr;
		if(HighSevStr != NULL) delete [] HighSevStr;
		if(hostExpr != NULL) delete [] hostExpr;
		if(volExpr != NULL) delete [] volExpr;
	}
};

class CFS
{
private:
	wchar_t *hostName;
	wchar_t *volName;
	ULONGLONG totSize;
	ULONGLONG freeMegs;
	double utilPercent;
    wchar_t *platformName;
	
	
	int FreeMegsThreshState;	//0 normal, 1 warning, 2 high
	ULONGLONG HighFreeMegsThresh;
	ULONGLONG WarnFreeMegsThresh;

	int UtilThreshState;		//0 normal, 1 warning, 2 high
	double	HighUtilThresh;
	double	WarnUtilThresh;

	wchar_t *warnSevStr;
	wchar_t *highSevStr;

public:
	inline const wchar_t *GetHostName() { return hostName; }
	inline const wchar_t *GetVolumeName() { return volName; }

	bool bThreshChanged;		// when true, threshold have to be reevaluated

	CFS(const wchar_t *HostName, const wchar_t *VolumeName, const wchar_t *platform, const ULONGLONG MaxSize = 0, const ULONGLONG nFreeMegs = 0, const double UsedPercent = 0.0f) 
		: totSize(MaxSize), freeMegs(nFreeMegs), utilPercent(UsedPercent), bThreshChanged(true), FreeMegsThreshState(0),
		HighFreeMegsThresh(0), WarnFreeMegsThresh(0), UtilThreshState(0), HighUtilThresh(0.0f), WarnUtilThresh(0.0f), warnSevStr(NULL), highSevStr(NULL), platformName(NULL)
	{
		size_t strLen = wcslen(HostName)+1;
		hostName = new wchar_t[strLen];
		memcpy(hostName, HostName, strLen*sizeof(wchar_t));

		strLen = wcslen(VolumeName)+1;
		volName = new wchar_t[strLen];
		memcpy(volName, VolumeName, strLen*sizeof(wchar_t));

        strLen = wcslen(platform)+1;
        platformName = new wchar_t[strLen];
        memcpy(platformName, platform, strLen*sizeof(wchar_t));

		SetValues(MaxSize, nFreeMegs, UsedPercent);
	}

	~CFS()
	{
		if(volName != NULL) delete [] volName;
		if(hostName != NULL) delete [] hostName;
		if(warnSevStr != NULL) delete [] warnSevStr;
		if(highSevStr != NULL) delete [] highSevStr;
        if(platformName != NULL) delete [] platformName;
	}

	

	inline wchar_t* BuildMessageString(
		const wchar_t *threshType, 
		int oldState, 
		int newState)
	{
		wchar_t *tmpStr = new wchar_t [2048];
		wchar_t *p = tmpStr;
		p = wcsapp(p, L"opcmsg a=UGMon o=Threshold -node=");
		p = wcsapp(p, hostName);
		p = wcsapp(p, L" msg_t=\"Hostname='");
		p = wcsapp(p, hostName);
		p = wcsapp(p, L"' threshType='");
		p = wcsapp(p, threshType);
        p = wcsapp(p, L"' platform='");
		p = wcsapp(p, platformName);
		if(newState == 2)
			p = wcsapp(p, L"' newState='HIGH'");
		else if(newState == 1)
			p = wcsapp(p, L"' newState='WARN'");
		else
			p = wcsapp(p, L"' newState='NORM'");

		if(oldState == 2)
			p = wcsapp(p, L" oldState='HIGH' volume='");
		else if(oldState == 1)
			p = wcsapp(p, L" oldState='WARN' volume='");
		else
			p = wcsapp(p, L" oldState='NORM' volume='");
		p = wcsapp(p, this->volName);
		p = wcsapp(p, L"' ");
		wchar_t *tmpStr2 = new wchar_t[250];	
		swprintf(tmpStr2, 249, L"FreeMB='%I64u' totMB='%I64u' percent='%.2f' ThreshMBWarn='%I64u' ThreshMBHigh='%I64u' ThreshUtilWarn='%.2f' ThreshUtilHigh='%.2f'", 
			freeMegs, this->totSize, this->utilPercent, WarnFreeMegsThresh, HighFreeMegsThresh,
			WarnUtilThresh, HighUtilThresh);
		p = wcsapp(p, tmpStr2);
		delete [] tmpStr2;
		p = wcsapp(p, L"\" sev=");
		if(newState == 0)
			p = wcsapp(p, L"Normal");
		else if(newState == 1)
			p = wcsapp(p, warnSevStr);
		else
			p = wcsapp(p, highSevStr);

		return tmpStr;
	}

	inline void SetValues(const ULONGLONG TotalSpace, const ULONGLONG FreeSpace, double UtilPercent)
	{
		if(bThreshChanged)
			GetThresholds();
		
		totSize=TotalSpace;
		freeMegs=FreeSpace;
		utilPercent = UtilPercent;

		// have we crossed the util threshold?
		int newState = 0;
		if(TotalSpace == 0)
			newState = 0;	// normal;
		else if( utilPercent > WarnUtilThresh && WarnUtilThresh != 0 && utilPercent <= HighUtilThresh)
			newState = 1;
		else if(utilPercent > HighUtilThresh && HighUtilThresh != 0)
			newState = 2;
		if(newState != UtilThreshState)
		{
			FreeMegsThreshState = 0;
			dn(L"THRESH ", L"Util threshold for volume %s on host %s, changed state from %s to %s, currently there %I64u out of %I64u MB free (%.2f%% util), threshold (warn/high): %.2f/%.2f\r\n", 
				volName, hostName, ThreshTypes[UtilThreshState], ThreshTypes[newState],
				freeMegs, totSize, utilPercent, WarnUtilThresh, HighUtilThresh);

			wchar_t *wcsCmd = BuildMessageString(L"Utilization", UtilThreshState, newState);

			ExecObj *newCmd = new ExecObj();
			newCmd->setCmdStr(wcsCmd);
			delete [] wcsCmd;
			CmdQueue.push(newCmd);
			dd(L"THRESH ", L"Queued command: %s\r\n", newCmd->GetCmdStr());
			// queue a command to notify the manager that the thresholds have changed
			UtilThreshState = newState;
		}
		else
		{
			// have we crossed the freespace threshold?
			newState = 0;
			if(TotalSpace == 0)
				newState = 0;	// normal;
			else if(freeMegs < HighFreeMegsThresh && HighFreeMegsThresh != 0)
				newState = 2;
			else if(freeMegs < WarnFreeMegsThresh && WarnFreeMegsThresh != 0)
				newState = 1;
			if(newState != FreeMegsThreshState)
			{
				UtilThreshState = 0;
				dn(L"THRESH ", L"Freespace threshold for volume %s on host %s, changed state from %s to %s, currently there %I64u out of %I64u MB free (%.2f%% util), threshold (warn/high) %I64u/%I64u\r\n", 
					volName, hostName, ThreshTypes[FreeMegsThreshState], ThreshTypes[newState],
					freeMegs, totSize, utilPercent, WarnFreeMegsThresh, HighFreeMegsThresh);

				wchar_t *wcsCmd = BuildMessageString(L"Freespace", FreeMegsThreshState, newState);

				ExecObj *newCmd = new ExecObj();
				newCmd->setCmdStr(wcsCmd);
				delete [] wcsCmd;
				CmdQueue.push(newCmd);
				dd(L"THRESH ", L"Queued command: %s\r\n", newCmd->GetCmdStr());
				// queue a command to notify the manager that the thresholds have changed
				FreeMegsThreshState = newState;
			}
		}
	}

	inline void GetThresholds()
	{
		CThreshold *myThresh = GetThresholdForFS(this->hostName, this->volName, this->totSize);
		if(myThresh != NULL)
		{
			//wprintf(L"%I64u high thresh %I64u warn thresh\r\n", myThresh->HighFreeMegsThresh, myThresh->WarnFreeMegsThresh);
			this->HighFreeMegsThresh = myThresh->HighFreeMegsThresh;
			this->WarnFreeMegsThresh = myThresh->WarnFreeMegsThresh;
			this->HighUtilThresh = myThresh->HighUtilThresh;
			this->WarnUtilThresh = myThresh->WarnUtilThresh;
			if(this->warnSevStr != NULL)
				delete [] this->warnSevStr;
			this->warnSevStr = new wchar_t[wcslen(myThresh->WarnSevStr)+1];
			wcscpy(this->warnSevStr, myThresh->WarnSevStr);

			if(this->highSevStr != NULL)
				delete [] this->highSevStr;
			this->highSevStr = new wchar_t[wcslen(myThresh->HighSevStr)+1];
			wcscpy(this->highSevStr, myThresh->HighSevStr);
		}
		else
		{
			this->HighFreeMegsThresh = 0;
			this->WarnFreeMegsThresh = 0;
			this->HighUtilThresh = 0.0f;
			this->WarnUtilThresh = 0.0f;
			if(this->warnSevStr != NULL)
			{
				delete [] this->warnSevStr;
				this->warnSevStr = NULL;
			}
			if(this->highSevStr != NULL)
			{
				delete [] this->highSevStr;
				this->highSevStr = NULL;
			}
		}
	}
};

CThreshold *GetThresholdForFS(const wchar_t *hostName, const wchar_t *volumeName, ULONGLONG fsSizeMB)
{
	CStrMatch hostMatch, volMatch;

	std::vector<CThreshold*>::iterator it = Thresholds.begin();
	while (it != Thresholds.end())
	{
		//wprintf(L"Matching %s on %s\r\n",(*it)->hostExpr, hostName); 
		hostMatch.SetExpr((*it)->hostExpr);
		if(hostMatch.MatchString(hostName) == 0)
		{
			if(    ((*it)->minFSSize == 0 ||  fsSizeMB >= (*it)->minFSSize ) &&
				((*it)->maxFSSize == 0 || fsSizeMB <= (*it)->maxFSSize ))	// if the size of the filesystem is bigger than minfssize and less than maxfssize
			{
				//wprintf(L"\tMatching %s on %s\r\n",(*it)->volExpr, volumeName); 
				// hostname matched, lets try the volume name
				volMatch.SetExpr((*it)->volExpr);
				if(volMatch.MatchString(volumeName) == 0)
					return (*it);
			}
		}
		it++;
	}
	return NULL;	// no thresholds matched
}

void UpdateThresholds(CSession *session)
{
	//dn(L"DB    ", L"Loading thresholds\r\n");
	while(!Thresholds.empty())
	{
		delete Thresholds.back();
		Thresholds.pop_back();
	}
	// Read all thresholds from the database

	CFSThresholds *threshRow = new CFSThresholds();

	threshRow->OpenAll(session);

	while (threshRow->MoveNext() == S_OK)
	{
		dd(L"DB    ", L"Found threshold %s:%s warn/high mb(%i/%i) util(%.2f/%.2f) - minFSSizeMB: %i, maxFSSizeMB: %i\r\n",
			threshRow->m_HostExpression, threshRow->m_VolumeExpression,
			threshRow->m_TreshFreeMBWarn, threshRow->m_ThreshFreeMBHigh,
			threshRow->m_ThreshUtilWarn, threshRow->m_ThreshUtilHigh,
			threshRow->m_MinFSSizeMB, threshRow->m_MaxFSSizeMB);
		Thresholds.push_back(new CThreshold(threshRow));
	}
	
	threshRow->CloseAll();
	delete threshRow;

	dn(L"DB    ", L"Loaded %u thresholds\r\n", Thresholds.size());

	// flag all filesystems so that they reevaluate thresholds upon next run
	std::vector<CFS*>::iterator fsIt = FileSystems.begin();
	while (fsIt != FileSystems.end())
	{
		(*fsIt)->bThreshChanged = true;
		fsIt++;
	}
}

CFS *GetFSByName(const wchar_t *Host, const wchar_t *volume)
{
	std::vector<CFS*>::iterator fsit = FileSystems.begin();
	while (fsit != FileSystems.end())
	{
		if(	wcsicmp(Host,   (*fsit)->GetHostName())   == 0  &&
			wcsicmp(volume, (*fsit)->GetVolumeName()) == 0 )
			return (*fsit);
		fsit++;
	}
	return NULL;
}

#endif