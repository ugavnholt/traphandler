#pragma once

#include "snmpOid.h"

char *GetFieldLength(char *asn1Data, unsigned int &length);
char *ParseIP(char *asn1Data, unsigned int &Val, unsigned int fieldLen);
char *ParseInt(char *asn1Data, unsigned int &Val, unsigned int fieldLen);
char *ParseBigInt(char *asn1Data, ULONGLONG &Val, unsigned int fieldLen);
char *ParseOid(char *asn1Data, CSnmpOid *Val, unsigned int fieldLen);
char *ParseBool(char *asn1Data, bool &Val, unsigned int fieldLen);
char *ParseStr(char *asn1Data, wchar_t *strVal, unsigned int fieldLen);

// TODO Implement range check
inline char *GetFieldLength(char *asn1Data, unsigned int &length)
{
	char *pTmp = asn1Data;
	length = 0;
	// Short form if bit 8=0
	if(((*asn1Data) & 0x80) == 0x00)
	{
		length = *asn1Data;
		pTmp++;
	}
	else
	{
		// bit 1-7 specifies the number of octets in length
		unsigned int nLen = (*pTmp) & 0x7f;
		pTmp++;

		for(unsigned int i=0; i < nLen; i++)
		{
			length = length << 8;
			length += (unsigned char)*pTmp;
			pTmp++;
		}
	}

	asn1Data++;
	return pTmp;
}

inline char *ParseIP(char *asn1Data, unsigned int &Val, unsigned int fieldLen)
{
	char *pTmp = asn1Data;
	Val = 0;

	for(unsigned int i=0; i < fieldLen; i++)
	{
		Val = Val << 8;
		Val += (unsigned char)*pTmp;
		pTmp++;
	}
	
	return pTmp;
}

inline char *ParseInt(char *asn1Data, unsigned int &Val, unsigned int fieldLen)
{
	if(fieldLen > 4)
		return NULL;
	Val = 0;
	char *pTmp = asn1Data;

	for(unsigned int i=0; i < fieldLen; i++)
	{
		Val = Val << 8;
		Val += (unsigned char)*pTmp;
		pTmp++;
	}

	return pTmp;
}

inline char *ParseBigInt(char *asn1Data, ULONGLONG &Val, unsigned int fieldLen)
{
	if (fieldLen > 8) 
		return NULL;
	Val = 0;
	char *pTmp = asn1Data;

	for(unsigned int i=0; i < fieldLen; i++)
	{
		Val = Val << 8;
		Val += (unsigned char)*pTmp;
		pTmp++;
	}

	return pTmp;
}

inline char *ParseOid(char *asn1Data, CSnmpOid *Val, unsigned int fieldLen)
{
	char *pTmp = asn1Data;
	Val->Oids = new unsigned int[fieldLen+1];	// we might create a little extra space here
	unsigned int tmpVal = 0;
	Val->nOids = 0;
#pragma warning (disable: 4018)
	while(pTmp-asn1Data < fieldLen)
	{
		tmpVal = 0;
		// Each value consist of the 7 least significant bits - bit 8 indicates whether more bits follows
		while(pTmp-asn1Data < fieldLen)
		{
			tmpVal = tmpVal << 7;
			tmpVal += ((unsigned char)*pTmp) & 0x7f;

			pTmp ++;
			if((*(pTmp-1) & 0x80) != 0x80)
				break;
		}

		if(tmpVal == 0x2b && Val->nOids == 0)	// 2B as the first value means .1.3
		{
			Val->Oids[0] = 0x01;
			Val->Oids[1] = 0x03;
			Val->nOids = 2;
		}
		else
		{
			Val->Oids[Val->nOids] = tmpVal;
			Val->nOids++;
		}
	}

	return pTmp;
}

inline char *ParseBool(char *asn1Data, bool &Val, unsigned int fieldLen)
{
	if(*(asn1Data+1) == 0)
		Val = false;
	else
		Val = true;
	return (asn1Data+2);
}

inline char *ParseStr(char *asn1Data, wchar_t *strVal, unsigned int fieldLen)
{
	wchar_t *pTmp = strVal;
	char	*pDat = asn1Data;

	for(unsigned int i=0; i < fieldLen; i++)
	{
		*pTmp = *pDat;
		pTmp++;
		pDat++;
	}

	*pTmp = L'\0';
	return pDat;
}


