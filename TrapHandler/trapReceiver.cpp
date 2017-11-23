#include "stdafx.h"
#include "debugheaders.h"
#include "utils.h"
#include "trapReceiver.h"
#include "asn1parsers.h"

static const wchar_t *Facility = L"SNMP  ";

static char sockBuf[65535];
static CTrapReceiver *pReceiver = NULL;
static volatile long __declspec(align(16)) nReceivedTraps = 0;
static std::vector<SSnmpPacket*> *activeBuf = NULL;

concurrency::concurrent_queue<SSnmpPacket *> pQueue;

DWORD TrapReceiveTimer = 0;

DWORD WINAPI ListenerFn( LPVOID pArg);


void CTrapReceiver::Start()
{
	unsigned int portNum = this->nListenPort;
	bConnected = false;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ListenerFn, (LPVOID)&portNum, 0, &dwThreadId);

	if(hThread == INVALID_HANDLE_VALUE)
	{
		dc(Facility, L"Unable to create SNMP listener thread\r\n");
		closesocket(ListenSocket);
		return;
	}

	/*
	THREAD_PRIORITY_TIME_CRITICAL 
	THREAD_PRIORITY_HIGHEST 
	THREAD_PRIORITY_ABOVE_NORMAL 
	THREAD_PRIORITY_NORMAL 
	THREAD_PRIORITY_BELOW_NORMAL 
	THREAD_PRIORITY_LOWEST 
	THREAD_PRIORITY_IDLE
	*/
	// SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);

	bConnected = true;

	return;
}

bool CTrapReceiver::Stop()
{
	// Cleanup the active buffer 
	if(activeBuf != NULL)
	{
		//wprintf(L"Clearing activeBuf, size: %u\n", activeBuf->size());
		while(!activeBuf->empty())
		{
			delete activeBuf->back();
			activeBuf->pop_back();
		}
		delete activeBuf;
		activeBuf = NULL;
	}

	if(!bConnected)
		return true;	// we are already disconnected
	if(ListenSocket != INVALID_SOCKET)
	{
		closesocket(ListenSocket);
		ListenSocket = INVALID_SOCKET;
	}

	WaitForSingleObject(hThread, 0);	// Wait for thread to exit when socket is closed

	return true;
}

bool CTrapReceiver::Pause()
{
	return false;
}

bool CTrapReceiver::Resume()
{
	return false;
}

CTrapReceiver::CTrapReceiver(unsigned short ListenPort, int maxBufLength) : 
	SendSock(INVALID_SOCKET), 
	nBufMaxLen(maxBufLength), 
	ListenSocket(INVALID_SOCKET), 
	bConnected(false), 
	bWsaStarted(false), 
	hThread(INVALID_HANDLE_VALUE)
{
	if(pReceiver != NULL)
		return;

	nListenPort = ListenPort;
	InitializeCriticalSection(&csBufLock);
	pReceiver = this;
	WORD wVersionRequested;
    WSADATA wsaData;
    int err;

	wVersionRequested = MAKEWORD(2, 2);

    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
#ifdef _DEBUG
        wprintf(L"WSAStartup failed with error: %d\n", err);
#endif
		dc(Facility, L"WSAStartup failed with error: %d\n", err);
		bWsaStarted = false;

		//TrapReceiveTimer = pTrace->CreateTimer(L"TRAPRECVTIME");
		return;
    }

	ListenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ListenSocket == INVALID_SOCKET) 
	{
#ifdef _DEBUG
		wprintf(L"Error at socket(): %ld\n", WSAGetLastError());
#endif
		dc(Facility, L"Socket error: %ld\r\n", WSAGetLastError());
		WSACleanup();
		bWsaStarted = false;
		return;
	}

	SendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	bWsaStarted = true;
}

CTrapReceiver::~CTrapReceiver()
{
// 	wprintf(L"MaxTrapReceiveTime: %I64u, resolution: %I64u\n", pTrace->GetTimerTime(TrapReceiveTimer), pTrace->GetTimerResolution());

	if(ListenSocket != INVALID_SOCKET)
	{
		closesocket(ListenSocket);
		WaitForSingleObject(hThread, 0);
	}

	if(SendSock != INVALID_SOCKET)
		closesocket(SendSock);

	if(bWsaStarted == true)
		WSACleanup();

	if(activeBuf != NULL)
		Stop();

	pReceiver = NULL;

	DeleteCriticalSection(&csBufLock);
}

unsigned long CTrapReceiver::GetTrapCount()
{ 
	// InterLocked
	unsigned long RecvTraps = nReceivedTraps;
	// nReceivedTraps = 0;
	return RecvTraps;
}

void CTrapReceiver::ResetTrapCount()
{
	InterlockedExchange((LONG*)&nReceivedTraps, 0);
}

void CTrapReceiver::SendAck(SSnmpPacket* srcPacket, char* reqTypePos)
{
	*reqTypePos = (unsigned char)SNMP_RESPONSE;
	sockaddr_in service;
	memset((void *)&service, 0, sizeof(struct sockaddr_in));
	service.sin_family = AF_INET;
	//service.sin_addr.s_addr = htonl(srcPacket->srcAddr);
	service.sin_addr.s_addr = srcPacket->srcAddr;
	service.sin_port = srcPacket->srcPort;
	sendto(this->SendSock, srcPacket->buf, srcPacket->packetSize, 0, (sockaddr*)&service, sizeof(sockaddr_in));
}

///////////////////////////////////////////////////
// Function to listen for traps

DWORD WINAPI ListenerFn( LPVOID pArg)
{
	if(pReceiver == NULL)
		return false;
	if(!pReceiver->WsaStarted())
		return false;	// We have not been initialized correctly

	SOCKET socket = pReceiver->GetSocket();
	

	unsigned short portNum = pReceiver->nListenPort;

	//wprintf(L"Starting listener on port %u, socket numer: %u\n", portNum, socket);
	//wprintf(L"traps address: %x\n", &nReceivedTraps);
	nReceivedTraps = 0;

	//----------------------
	// The sockaddr_in structure specifies the address family,
	// IP address, and port for the socket that is being bound.
	sockaddr_in service;

	memset((void *)&service, 0, sizeof(struct sockaddr_in));
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = htonl(INADDR_ANY);
	service.sin_port = htons(portNum);

	bind( socket, (SOCKADDR*) &service, sizeof(service));	// Bind return error here, vista bug?

	sockaddr_in SenderAddr;
	int nRecv=1;
	int SenderAddrSize = sizeof(SenderAddr);
	while (nRecv > 0)
	{
		nRecv = recvfrom(socket, sockBuf, sizeof(sockBuf), 0, (SOCKADDR*) &SenderAddr, &SenderAddrSize);
		if(nRecv > 0 && nRecv < sizeof(sockBuf))
		{
			//pTrace->StartTimer(TrapReceiveTimer);
			// wprintf(L"reveivedtrap\n");
			//SenderAddrSize = sizeof(SenderAddr);

			// Save the receivetime
			SSnmpPacket *newPacket = new SSnmpPacket();
			GetSystemTimeAsFileTime(&newPacket->ftRecvTime);
			
			// Save the source address
			newPacket->srcAddr = SenderAddr.sin_addr.s_addr;

			// Save the client port (for acknowledgement)
			newPacket->srcPort = SenderAddr.sin_port;

			// Save the buffer length
			newPacket->packetSize = nRecv;

			// Build the raw message
			newPacket->buf = new char[nRecv];
			memcpy(newPacket->buf, sockBuf, nRecv);

			pQueue.push(newPacket);
			
			InterlockedIncrement(&nReceivedTraps);
			//pTrace->StopTimer(TrapReceiveTimer);
		}
	}

	 if(nRecv == 0xffffffff)
		 dw(Facility, L"Listener socket closed, Frontend is not listening for SNMP traps\r\n");
	
	//pReceiver->NotConnected();

	return 0;
}
