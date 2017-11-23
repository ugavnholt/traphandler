#ifndef __CORRELATOR_HEAD
#define __CORRELATOR_HEAD

#include "TraceEngine.h"
#include "service.h"
#include <list>
#include "TrapReceiver.h"
#include "TimerService.h"
#include "HostControl.h"
#include "utils.h"
#include "AgentEventFactory.h"
#include "TrapHandlerModel.h"
#include "CmdQueue.h"

#pragma comment (lib, "Version.lib") // to read the VS_VERSION_INFO structure
#pragma warning (disable :4996)

static const wchar_t *Facility		= L"MAIN  ";
const wchar_t *NoPlatform			= L"UNKNOWN";
const wchar_t *VersionPre11			= L"pre-1.11";
const wchar_t *DefaultMsgGrp		= L"OS_";

const wchar_t *SevNormal			= L"Normal";
const wchar_t *SevWarning			= L"Warning";
const wchar_t *SevMinor				= L"Minor";
const wchar_t *SevMajor				= L"Major";
const wchar_t *SevCritical			= L"Critical";
const wchar_t *DefaultSeverity		= SevMajor;



#include "Thresholds.h"

////////////////////////////////
// Globals

HINSTANCE hInst = 0;
wchar_t wcsExeFileName[256];
wchar_t wcsServiceName[256];
wchar_t wcsServiceLabel[256];
wchar_t wcsServiceDescr[2049];
wchar_t wcsLocalHostName[256];	// our local host name
DWORD dwGracePeriodSecs = 120;

#include "globalConf.h"
////////////////////////////////

namespace traphandler
{
	class TrapHandler;
	TrapHandler *pInstance = nullptr;
	// Main application class
	class TrapHandler
	{
	public:
		wchar_t *versionString = nullptr;
		wchar_t *productName = nullptr;
		void PrintError(int IDSString);
		bool ReadConf();
		bool CreateConf(const wchar_t *installPath, const wchar_t *instanceName, const wchar_t *dbName);
		bool ServiceDelete();
		bool ServiceInstall(const wchar_t *pathName);
		// initializes the application running as a service
		bool StartAsService();
		// initializes the application running in the console
		bool StartAsConsole();
		static bool Initialize();
		void Shutdown();
		static TrapHandler &GetInstance() { return *pInstance; }
	private:
		bool GetVersionInfo();
		void LoadGlobalStrings();
		bool consoleMode = true;
		bool UpdateConfig();
		HANDLE hQuitEvent = INVALID_HANDLE_VALUE;

		TrapHandler() {}
		~TrapHandler() {}
		CmdQueue cmdQueue;
		TimerService timers;
		events::AgentEventFactory eventFactory;
		model::TrapHandlerModel model;
		CTrapReceiver *trapReceiver = nullptr;

		void processTraps();

		static DWORD HandlerMainFn(LPDWORD pParam);
	};

} // namespace traphandler

#pragma warning (disable: 4312)

#endif