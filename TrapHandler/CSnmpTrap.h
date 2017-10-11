#pragma once

#include "snmpOid.h"
#include "asn1parsers.h"
#include <vector>

#define SNMP_VERSION1		0x00
#define SNMP_VERSION2C		0x01

#define	SNMP_GETREQ			0xA0
#define SNMP_RESPONSE		0xA2	// this is what we send, when acknowledging a trap
#define SNMP_SETREQ			0xA3
#define SNMP_V1TRAP			0xA4
#define SNMP_V2INFORM		0xA6	// Requires ack
#define SNMP_V2TRAP			0xA7


#define SNMP_TYPE_BOOL		0x01
#define SNMP_TYPE_INT		0x02
#define SNMP_TYPE_BITSTR	0x03
#define SNMP_TYPE_STR		0x04
#define SNMP_TYPE_NULL		0x05
#define SNMP_TYPE_OID		0x06

#define SNMP_TYPE_RELOID	0x0D
#define SNMP_TYPE_SEQ		0x30

#define SNMP_TYPE_IPADDR	0x40
#define SNMP_TYPE_TIME		0x43

// Guessing here
#define SNMP_TYPE_CNT32		0x41
#define SNMP_TYPE_GAUGE32	0x42
#define SNMP_TYPE_OPAQUE	0x44
#define SNMP_TYPE_INT64		0x46

struct SSnmpPacket
{
	SSnmpPacket() : buf(NULL) { }
	~SSnmpPacket() { if (buf != NULL) delete[] buf; }
	unsigned long	srcAddr;
	unsigned short	srcPort;
	FILETIME		ftRecvTime;
	unsigned int	packetSize;
	char			*buf;
};


class CSnmpTrapVar
{
public:
	CSnmpTrapVar() : oid(NULL), oidVal(NULL), wcsVal(NULL), iValueType(0), iVal(0), bVal(false), ullVal(0), wcsTmpVal(NULL) {}
	~CSnmpTrapVar()
	{
		if (oid != NULL) delete oid;
		if (oidVal != NULL) delete oidVal;
		if (wcsVal != NULL) delete[] wcsVal;
		if (wcsTmpVal != NULL) delete[] wcsTmpVal;
	}

	CSnmpOid		*oid;			// The oid name
	int				iValueType;		// The type of the variable
	CSnmpOid		*oidVal;
	wchar_t			*wcsVal;
	unsigned int	iVal;
	bool			bVal;			// bool
	ULONGLONG		ullVal;			// int32

	operator const wchar_t*();		// Convert value to wide string

private:
	wchar_t			*wcsTmpVal;
};

// Class to hold entire SNMP message
class CSnmpTrap
{
public:
	CSnmpTrap(SSnmpPacket *packet);
	~CSnmpTrap();
	bool Decode(char *pBuf);
	unsigned long	ulSourceIP;
	uint64_t		ullSourceTime;
	CSnmpOid		*TrapOid;
	FILETIME		ftRecvTime;
	int				TrapType;
	unsigned int	Version;
	unsigned int	PacketLen;
	wchar_t			*community;
	unsigned int	RequestID;
	unsigned int	Error;
	unsigned int	ErrorIndex;
	unsigned int	clientPort;

	unsigned int	SnmpSourceIP;
	char*			pReqType;	// pointer to the position in pBuf where we need to set the request type if acknowledging
	std::vector<CSnmpTrapVar*>	varArgs;
};

inline CSnmpTrapVar::operator const wchar_t *()
{
	if (wcsTmpVal != NULL)
		return wcsTmpVal;

	if (this->iValueType == SNMP_TYPE_BOOL)
	{
		wcsTmpVal = new wchar_t[6];
		if (this->bVal == true)
			wcscpy(wcsTmpVal, L"true");
		else
			wcscpy(wcsTmpVal, L"false");
	}
	else if (this->iValueType == SNMP_TYPE_INT || this->iValueType == SNMP_TYPE_TIME)
	{
		if (this->ullVal > 0)	// its a 64-bit integer
		{
			wcsTmpVal = new wchar_t[44];
			_ui64tow(ullVal, wcsTmpVal, 10);
		}
		else
		{
			wcsTmpVal = new wchar_t[22];
			_ltow(iVal, wcsTmpVal, 10);
		}
	}
	else if (this->iValueType == SNMP_TYPE_STR)
	{
		wcsTmpVal = new wchar_t[wcslen(wcsVal) + 1];
		wcscpy(wcsTmpVal, wcsVal);
	}
	else if (this->iValueType == SNMP_TYPE_NULL)
	{
		wcsTmpVal = new wchar_t[5];
		wcscpy(wcsTmpVal, L"NULL");
	}
	else if (this->iValueType == SNMP_TYPE_OID)
	{
		wcsTmpVal = new wchar_t[wcslen((wchar_t*)*this->oidVal) + 1];
		wcscpy(wcsTmpVal, (wchar_t*)*this->oidVal);
	}
	else if (this->iValueType == SNMP_TYPE_IPADDR)
	{
		wcsTmpVal = new wchar_t[16];
		wchar_t *pTmp = wcsTmpVal;
		unsigned int andVal = 0xff000000;
		unsigned int shiftVal = 24;

		for (int i = 0; i < 4; i++)
		{
			unsigned int tmpVal = (this->iVal & (0xff000000 << (8 * i))) >> (shiftVal - (8 * i));
			_itow(tmpVal, pTmp, 10);
			if (this->iVal >= 100)
				pTmp = pTmp + 3;
			else if (this->iVal >= 10)
				pTmp = pTmp + 2;
			else
				pTmp++;
			*pTmp++ = L'.';
		}
		*(pTmp - 1) = L'\0';
	}
	else
		return NULL;
	return wcsTmpVal;
}

// Parse an asn.1 sequence and construct a packet from it
inline CSnmpTrap::CSnmpTrap(SSnmpPacket *packet) :

	ftRecvTime(packet->ftRecvTime),
	PacketLen(packet->packetSize),
	Version(65535),
	community(NULL),
	TrapOid(NULL),
	pReqType(NULL)
{
	//wprintf(L"raw source: %x\n", packet->srcAddr);
	ulSourceIP = ntohl(packet->srcAddr);
	//wprintf(L"converted source: %x\n", ulSourceIP);
}

inline CSnmpTrap::~CSnmpTrap()
{
	if (community != NULL) delete[] community;
	if (this->TrapOid != NULL) delete this->TrapOid;
	while (!varArgs.empty())
	{
		delete varArgs.back();
		varArgs.pop_back();
	}
}

inline bool CSnmpTrap::Decode(char *pBuf)
{
	char *pTmp = pBuf;

	//////////////////
	// Get Envelope
	if (*pTmp != 0x30)
		return false;	// Without the envelope its not a trap

	pTmp++;

	unsigned int fieldLen;
	pTmp = GetFieldLength(pTmp, fieldLen);

	if (fieldLen != (this->PacketLen - (pTmp - pBuf)))
	{
		// wprintf(L"Invalid length specified in SNMP packet packet length: %x, reported length: %x\n", this->PacketLen, msgLen);
		return false;	// Length does not match packet
	}

	// next field is the SNMP version
	if (*pTmp != SNMP_TYPE_INT)
		return false;	// integer type expected, but not found

	pTmp++;

	//////////////////
	// Get Version
	pTmp = GetFieldLength(pTmp, fieldLen);
	if (fieldLen <= 4)
		pTmp = ParseInt(pTmp, this->Version, fieldLen);
	else
		return false;	// Only a small integer should appear in version

						//////////////////
						// Get community string

	if (*pTmp != SNMP_TYPE_STR)
		return false;	// string expected, but not found
	pTmp++;
	pTmp = GetFieldLength(pTmp, fieldLen);
	this->community = new wchar_t[fieldLen + 1];	// Reserve space for sommunity

	pTmp = ParseStr(pTmp, community, fieldLen);

	//////////////////
	// Get request type

	this->pReqType = pTmp;	// store the position of the requesttype for acknowleding

	if ((unsigned char)*pTmp == SNMP_V1TRAP || (unsigned char)*pTmp == SNMP_V2INFORM || (unsigned char)*pTmp == SNMP_V2TRAP)	// SNMP v1 trap
	{
		this->TrapType = (unsigned char)*pTmp;
	}
	else
		return false;	// we are not an agent

	pTmp++;

	//////////////////
	// Get PDU length
	pTmp = GetFieldLength(pTmp, fieldLen);

	///////////////////////////////
	//
	// V1 Trap Parser
#pragma region V1

	if (TrapType == SNMP_V1TRAP)
	{
		// Following attributes are not contained in V1 traps
		this->RequestID = 0;
		this->Error = 0;
		this->ErrorIndex = 0;

		////////////////////////////
		// Enterprise ID
		if (*pTmp != SNMP_TYPE_OID)
			return false;	// we should have the enterprise ID here
		pTmp++;

		pTmp = GetFieldLength(pTmp, fieldLen);

		this->TrapOid = new CSnmpOid();
		pTmp = ParseOid(pTmp, TrapOid, fieldLen);

		////////////////////////////
		// source address

		if (*pTmp != SNMP_TYPE_IPADDR || *(++pTmp) != 4)
			return false;	// This should have been an IP address

		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseIP(pTmp, SnmpSourceIP, fieldLen);

		////////////////////////////
		// generic ID
		CSnmpOid *genericID = new CSnmpOid();
		if (*pTmp != SNMP_TYPE_INT || *(++pTmp) != 1)
		{
			delete genericID;
			return false;	// Invalid type, or invalid length
		}

		unsigned int GenericID;
		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseInt(pTmp, GenericID, fieldLen);

		if (GenericID == 6)
		{
			genericID->nOids = 2;
			genericID->Oids = new unsigned int[2];	// room for generic and specific id
			genericID->Oids[0] = 0;
		}
		else
		{
			genericID->nOids = 1;
			genericID->Oids = new unsigned int[1];
			genericID->Oids[0] = GenericID + 1;
		}

		////////////////////////////
		// Specific ID

		unsigned int SpecificID;
		if (*pTmp != SNMP_TYPE_INT)
		{
			delete genericID;
			return false;	// Invalid type, or invalid length
		}
		pTmp++;

		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseInt(pTmp, SpecificID, fieldLen);
		if (GenericID == 6)
			genericID->Oids[1] = SpecificID;

		*this->TrapOid += *genericID;
		delete genericID;

		////////////////////////////
		// Source time
		if (*pTmp != SNMP_TYPE_TIME)
			return false;	// we need the timestamp

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);
		if (fieldLen <= 4)	// we do this check, even if 64 bit integers are not supported by v1
		{
			unsigned int tmpTime;
			pTmp = ParseInt(pTmp, tmpTime, fieldLen);
			this->ullSourceTime = tmpTime;
		}
		else
			pTmp = ParseBigInt(pTmp, this->ullSourceTime, fieldLen);

		// Skip the varbinds container
		if (*pTmp != SNMP_TYPE_SEQ)
			return false;	// should have been a tag here
		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);

		// The nextsection is the first varbind
	}
#pragma endregion
	///////////////////////////////
	//
	// V2 Trap Parser
#pragma region V2
	else
	{
		////////////////////////
		// Request ID
		if (*pTmp != 0x02)
			return false;	// We didn't find an integer

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseInt(pTmp, this->RequestID, fieldLen);

		////////////////////////
		// Error
		if (*pTmp != 0x02)
			return false;	// We didn't find an integer

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseInt(pTmp, this->Error, fieldLen);

		////////////////////////
		// Error index
		if (*pTmp != 0x02)
			return false;	// We didn't find an integer

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseInt(pTmp, this->ErrorIndex, fieldLen);

		// Skip trapOid Sequence
		if (*pTmp != SNMP_TYPE_SEQ)
			return false;	// Sequence tag for all varbinds

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);

		//////////////////////////
		// First varbind (Timestamp)
		if (*pTmp != SNMP_TYPE_SEQ)
			return false;	// Sequence of the next varbind

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);	// Length of varbind

		if (*pTmp != SNMP_TYPE_OID)
			return false;

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);	// Length of timestamp name oid

		this->TrapOid = new CSnmpOid();
		pTmp = ParseOid(pTmp, TrapOid, fieldLen);
		// wprintf(L"Time OID Name: %s\n", (wchar_t*)*TrapOid);

		delete TrapOid;
		TrapOid = new CSnmpOid();

		if (*pTmp != SNMP_TYPE_TIME)
			return false;	// We expect the timestamp here
		pTmp++;

		pTmp = GetFieldLength(pTmp, fieldLen);	// Length of timestamp
		if (fieldLen <= 4)	// we do this check, even if 64 bit integers are not supported by v1
		{
			unsigned int tmpTime;
			pTmp = ParseInt(pTmp, tmpTime, fieldLen);
			this->ullSourceTime = tmpTime;
		}
		else
			pTmp = ParseBigInt(pTmp, this->ullSourceTime, fieldLen);

		// Next trap sequence is the Trap oid
		if (*pTmp != SNMP_TYPE_SEQ)
			return false;
		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);	// Length of Trapoid name

												//////////////////////////
												// Get TrapOid
		if (*pTmp != SNMP_TYPE_OID)
			return false;	// Now we need another oid, which is the name of the event oid
		pTmp++;

		pTmp = GetFieldLength(pTmp, fieldLen);	// Length of name of Trapoid
		pTmp = ParseOid(pTmp, TrapOid, fieldLen);

		delete TrapOid;
		TrapOid = new CSnmpOid();

		if (*pTmp != SNMP_TYPE_OID)
			return false;	// This should be the trapOid

		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseOid(pTmp, TrapOid, fieldLen);
	}
#pragma endregion

	////////////////////////
	// varbind list
	//

	while (*pTmp == SNMP_TYPE_SEQ && pTmp < pBuf + this->PacketLen)
	{
		pTmp++;
		pTmp = GetFieldLength(pTmp, fieldLen);

		// get the varname
		if (*pTmp != SNMP_TYPE_OID)
			return false;
		pTmp++;

		CSnmpTrapVar *newVar = new CSnmpTrapVar();
		newVar->oid = new CSnmpOid();

		pTmp = GetFieldLength(pTmp, fieldLen);
		pTmp = ParseOid(pTmp, newVar->oid, fieldLen);

		newVar->iValueType = (unsigned char)*pTmp;

		if (*pTmp == SNMP_TYPE_BOOL)
		{
			pTmp++;
			pTmp = GetFieldLength(pTmp, fieldLen);
			pTmp = ParseBool(pTmp, newVar->bVal, fieldLen);
		}

		else if (*pTmp == SNMP_TYPE_INT || *pTmp == SNMP_TYPE_TIME)
		{
			pTmp++;
			pTmp = GetFieldLength(pTmp, fieldLen);

			if (fieldLen <= 4)
				pTmp = ParseInt(pTmp, newVar->iVal, fieldLen);
			else
				pTmp = ParseBigInt(pTmp, newVar->ullVal, fieldLen);
		}
		else if (*pTmp == SNMP_TYPE_STR)
		{
			pTmp++;
			pTmp = GetFieldLength(pTmp, fieldLen);
			newVar->wcsVal = new wchar_t[fieldLen + 1];
			pTmp = ParseStr(pTmp, newVar->wcsVal, fieldLen);
		}
		else if (*pTmp == SNMP_TYPE_NULL)
		{
			pTmp++;	// type tag
			pTmp++;	// content tag
		}
		else if (*pTmp == SNMP_TYPE_OID)
		{
			pTmp++;
			pTmp = GetFieldLength(pTmp, fieldLen);
			newVar->oidVal = new CSnmpOid();
			pTmp = ParseOid(pTmp, newVar->oidVal, fieldLen);
		}
		else if (*pTmp == SNMP_TYPE_IPADDR)
		{
			pTmp++;
			pTmp = GetFieldLength(pTmp, fieldLen);
			pTmp = ParseIP(pTmp, newVar->iVal, fieldLen);
		}
		else
			return false;	// Unable to decode unknown type

							//wprintf(L"pushing var\n");
		this->varArgs.push_back(newVar);
	}	// next var

	return true;	// Parsing successfull
}
