#include "stdafx.h"
#include "snmpOid.h"
//#include <stdlib.h>

void CSnmpOid::MakeWcsStr()
{
	if(Oids == NULL || wcsOid != NULL)
		return;	// nothing to convert, or we already did the job

	wcsOid = new wchar_t[MAXOIDSTRLEN+1];
	wchar_t *pTmp = wcsOid;

	*pTmp = L'.';	// Allways construct string with leading .
	pTmp++;

	// wprintf(L"wcsOid: %s\n", wcsOid);

	// wprintf(L"nOids: %u\n", nOids);

	wchar_t *wcsBuf = new wchar_t[16];

	for(unsigned int i=0; i < nOids; i++)
	{
		_ultow(Oids[i], wcsBuf, 10);
		wcscpy(pTmp, wcsBuf);
		pTmp = pTmp+wcslen(wcsBuf);
		//wprintf(L"wcsOid: %s\n", wcsOid);

		*pTmp = L'.';
		pTmp++;
		if(pTmp - wcsOid >= MAXOIDSTRLEN)	// prevent buffer overruns
		{
			delete [] wcsOid;
			wcsOid = NULL;
			delete [] wcsBuf;
			return;
		}
	}

	delete [] wcsBuf;
	*(pTmp-1) = L'\0';
}

void CSnmpOid::BuildFromWcsStr(const wchar_t *wcsNewOid)
{
	if(Oids != NULL) delete [] Oids;
	if(wcsOid != NULL) delete [] wcsOid;

	if(wcsNewOid == NULL)
	{
		Oids = NULL;
		wcsOid = NULL;
		nOids = 0;
		return;
	}

	size_t strLen = wcslen(wcsNewOid);

	if(strLen > MAXOIDSTRLEN)
	{
		wcsOid = NULL;
		nOids = 0;
		Oids = NULL;
		return;
	}

	// max oids that can be in string
	Oids = new unsigned int[(strLen+1)/2];

	const wchar_t	*pSrc = wcsNewOid;

	
	if(*wcsNewOid > L'0' && *wcsNewOid < L'9')
		strLen++;
	else if(*wcsNewOid == L'.')
		pSrc++;
	else	// we didn't have a valid start char
	{
		delete [] Oids; Oids = NULL;
		delete [] wcsOid; wcsOid = NULL;
		nOids = 0;
		return;
	}

	wcsOid = new wchar_t[strLen+1];		
	*wcsOid = L'.';
	wchar_t *pDst = wcsOid+1;

	nOids = 0;

	while (pSrc != L'\0')
	{
		// convert the digits to the next . to a unsigned int
		unsigned int tmpVal = 0;
		while(*pSrc >= L'0' && *pSrc <= L'9')
		{
			tmpVal = tmpVal * 10;
			tmpVal += (unsigned int)(*pSrc - L'0');
			*pDst = *pSrc;	// append character
			pSrc++;
			pDst++;
		}

		if(*(pDst-1) != L'.')	// if we had a value
		{
			Oids[nOids] = tmpVal;
			nOids++;	// we had a new oid
			if(*pSrc != L'.')	// Invalid character terminate parsing, but keep string
			{
				*pDst = L'\0';	// terminate string
				return;
			}
			else // we have a new number
			{
				*pDst = *pSrc;
				pDst++;
				pSrc++;
			}
		}
		else	// we didn't have a value
		{
			if(pDst == wcsOid+1)	// we found noting to parse
			{
				delete [] wcsOid; 
				wcsOid = NULL;
				delete [] Oids;
				Oids = NULL;
				nOids = 0;
			}
			else	// we parsed something, terminate string
			{
				*(pDst-1) = L'\0';		// Terminate string at the last valid number we found
			}

			//wprintf(L"Stopping parsing\n");
			return;
		}

		if(nOids > MAXOIDLEN)	// Oid length exceeded
		{
			delete [] wcsOid;
			wcsOid = NULL;
			delete [] Oids;
			Oids = NULL;
			nOids = 0;
			return;
		}
	}
}

void CSnmpOid::operator +=(const CSnmpOid &addOid)
{
	if(addOid.nOids == 0)
		return;	// nothing to add

	if(wcsOid != NULL) delete [] wcsOid; wcsOid = NULL;
	
	unsigned int *newOid = new unsigned int[this->nOids + addOid.nOids];

	memcpy(newOid, Oids, nOids*sizeof(unsigned int));
	memcpy(newOid+nOids, addOid.Oids, addOid.nOids * sizeof(unsigned int));
	nOids += addOid.nOids;

	if(Oids != NULL) delete [] Oids; 
	
	Oids = newOid;
}

void CSnmpOid::operator +=(const CSnmpOid *addOid)
{
	if(addOid->nOids == 0)
		return;	// nothing to add

	if(wcsOid != NULL) delete [] wcsOid; wcsOid = NULL;
	
	unsigned int *newOid = new unsigned int[this->nOids + addOid->nOids];

	memcpy(newOid, Oids, nOids*sizeof(unsigned int));
	memcpy(newOid+nOids, addOid->Oids, addOid->nOids * sizeof(unsigned int));
	nOids += addOid->nOids;

	if(Oids != NULL) delete [] Oids; 
	
	Oids = newOid;
}

bool CSnmpOid::operator ==(const wchar_t *wcsCompOid)
{
	return false;
}

bool CSnmpOid::operator ==(const CSnmpOid *CompOid)
{
	if(nOids != CompOid->nOids)
		return false;
	for(unsigned int i = 0; i < nOids; i++)
	{
		if(this->Oids[i] != CompOid->Oids[i])
			return false;
	}
	return true;
}