#ifndef __TRAPRECEIVE__HEAD
#define __TRAPRECEIVE__HEAD


#include "CSnmpTrap.h"
#include <vector>
#include <concurrent_queue.h>

struct SSnmpPacket;

extern concurrency::concurrent_queue<SSnmpPacket *> pQueue;

#pragma comment (lib, "Ws2_32.lib")


class CTrapReceiver
{
public:
	CTrapReceiver(unsigned short ListenPort, int maxBufLength = 10000);
	~CTrapReceiver();

	void Start();
	bool Stop();
	bool Pause();
	bool Resume();

	inline bool isConnected() const { return bConnected; }

	inline void NotConnected() { bConnected = false; }
	inline bool WsaStarted() { return bWsaStarted; }

	inline SOCKET GetSocket() { return ListenSocket; }
	inline int GetMaxBufLen() const { return nBufMaxLen; }
	void SendAck(SSnmpPacket* srcPacket, char* reqTypePos);

	// Returns and resets the trap counter
	unsigned long GetTrapCount();
	void ResetTrapCount();

	unsigned short nListenPort;
	SOCKET SendSock;

private:
	int nBufMaxLen;
	bool bWsaStarted;
	bool bConnected;
	HANDLE hThread;
	CRITICAL_SECTION	csBufLock;
	DWORD dwThreadId;

	SOCKET ListenSocket;
};


#endif