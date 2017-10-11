#ifndef __SNMPOID__HEAD
#define __SNMPOID__HEAD

#define MAXOIDSTRLEN	255
#define MAXOIDLEN		50

#include <wchar.h>

// #pragma comment (lib, "snmpOid.lib")

#pragma warning (disable : 4996)
class CSnmpOid
{
public:
	CSnmpOid() : wcsOid(NULL), Oids(NULL), nOids(0) {}
	
	CSnmpOid(const CSnmpOid &copyOid) : wcsOid(NULL)
	{
		if(copyOid.Oids == NULL)
		{
			Oids = NULL;
			nOids = 0;
			wcsOid = NULL;
			return;
		}
		else
		{
			nOids = copyOid.nOids;
			Oids = new unsigned int[nOids];
			memcpy(Oids, copyOid.Oids, nOids*sizeof(unsigned int));
		}

		if(copyOid.wcsOid != NULL)
		{
			wcsOid = new wchar_t[wcslen(copyOid.wcsOid)+1];
			wcscpy(wcsOid, copyOid.wcsOid);
		}
		else
			wcsOid = NULL;
	}

	CSnmpOid(const CSnmpOid *copyOid) : wcsOid(NULL)
	{
		if(copyOid->Oids == NULL)
		{
			Oids = NULL;
			nOids = 0;
			wcsOid = NULL;
			return;
		}
		else
		{
			nOids = copyOid->nOids;
			Oids = new unsigned int[nOids];
			memcpy(Oids, copyOid->Oids, nOids*sizeof(unsigned int));
		}

		if(copyOid->wcsOid != NULL)
		{
			wcsOid = new wchar_t[wcslen(copyOid->wcsOid)+1];
			wcscpy(wcsOid, copyOid->wcsOid);
		}
		else
			wcsOid = NULL;
	}

	~CSnmpOid()
	{
		if(wcsOid != NULL) delete [] wcsOid;
		if(Oids != NULL) delete [] Oids;
	}

	CSnmpOid(const wchar_t *NewWcsOid) : Oids(NULL), nOids(0), wcsOid(NULL)
	{
		BuildFromWcsStr(NewWcsOid);
	}

	operator wchar_t*()
	{
		if(wcsOid == NULL) 
			MakeWcsStr();
		
		return wcsOid;
		
	}

	operator const wchar_t*()
	{
		if(wcsOid != NULL) MakeWcsStr();
		return wcsOid;
	}

	void operator =(const wchar_t *wcsNewOid)
	{
		BuildFromWcsStr(wcsNewOid);
	}

	void operator =(const CSnmpOid &newOid)
	{
	}

	void operator +=(const CSnmpOid &addOid);
	void operator +=(const CSnmpOid *addOid);

	bool operator ==(const wchar_t *wcsCompOid);
	bool operator ==(const CSnmpOid *CompOid);

	unsigned int *Oids;
	unsigned int nOids;
	wchar_t *wcsOid;

private:
	void MakeWcsStr();
	void BuildFromWcsStr(const wchar_t *wcsNewOid);
};


#endif