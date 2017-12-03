#pragma once

#include "TestInterface.h"
#include <Windows.h>
#include "../TrapHandler/PocoOdbcStore.h"
#include "../TrapHandler/TrapHandlerModel.h"
#include <exception>
#include <assert.h>

namespace traphandler
{
	namespace tests
	{
		class ModelTests : public TestInterface
		{
		public:
			ModelTests()
				: Model(Store, 
					std::wstring(L"SOFTWARE\\Rubik Solutions\\TrapHandler"))
			{}
			~ModelTests()
			{
				Store.Disconnect();
			}
			bool RunTests() override
			{
				try
				{
					if (!Model.initialize())
						return false;
					if (!LoadConfig())
						return false;
					if (!LoadCache())
						return false;
					if (!TestStatus())
						return false;
					if (!CreateHosts())
						return false;
					if (!CreateFs())
						return false;
					Store.Disconnect();
				}
				catch (std::exception& e)
				{
					wprintf(L"Exception running tests: %S\r\n",
						e.what());
					return false;
				}
				return true;
			}

		private:
			bool LoadConfig()
			{
				wprintf(L"Loading proxy configuration...\r\n");
				Model.UpdateConfig(true);
				return true;
			}
			bool LoadCache()
			{
				wprintf(L"Loading cache objects...\r\n");
				Model.LoadCache();
				return true;
			}
			bool TestStatus()
			{
				wprintf(L"Testing status logic...\r\n");
				traphandler::model::Status testStatus;
				testStatus.setFlag(traphandler::model::eStatusValue::HBFailed);
				assert(testStatus.isSet(traphandler::model::eStatusValue::HBFailed));
				testStatus.setFlag(traphandler::model::eStatusValue::UtilHigh);
				assert(testStatus.isSet(traphandler::model::eStatusValue::HBFailed) && testStatus.isSet(traphandler::model::eStatusValue::UtilHigh));
				testStatus.clearFlag(traphandler::model::eStatusValue::HBFailed);
				assert(testStatus.isSet(traphandler::model::eStatusValue::UtilHigh));
				testStatus.clearFlag(traphandler::model::eStatusValue::UtilHigh);
				assert(testStatus.isNormal());
				return true;
			}
			bool CreateHosts()
			{
				wprintf(L"Creating 100 hosts...\r\n");
				for (int i = 0; i < 100; i++)
				{
					traphandler::model::Host NewHost;
					NewHost.architecture = L"64";
					wchar_t name[100];
					swprintf(name, L"hostxxx%i", i);
					NewHost.hostname = name;
					NewHost.ip = L"17.17.17.17";
					NewHost.ugmonVersion = L"1.17";
					NewHost.osVersionBuild = L"dunno";
					NewHost.osVersionMajor = 1;
					NewHost.osVersionMinor = 1;
					NewHost.servicePack = 27;
					NewHost.platform = L"Linux";
					NewHost.proxyHostName = L"myHost";
					NewHost.versionString = L"TestHost version 1.0.0";
					Model.CreateHost(NewHost);
				}
				return true;
			}
			bool CreateFs()
			{
				wprintf(L"Creating 100 file systems...\r\n");
				for (int i = 0; i < 100; i++)
				{
					traphandler::model::FileSystem newFs;
					newFs.name = L"/tmp";
					newFs.freeMb = 700;
					newFs.size = 1000;
					newFs.hostId = -1;
					newFs.thresholdId = -1;
					Model.CreateFileSystem(newFs);
				}
				return true;
			}
			traphandler::model::PocoOdbcStore Store;
			traphandler::model::TrapHandlerModel Model;
			traphandler::model::VoidNotification NotificationHandler;
		};
	}
}