#pragma once

#include "TestInterface.h"
#include <Windows.h>
#include "../TrapHandler/nanodbc.cpp"
#include "../TrapHandler/odbcStore.h"
#include "../TrapHandler/TrapHandlerModel.h"

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
					if (!ConnectDatabase())
						return false;
					if (!LoadConfig())
						return false;
					if (!LoadCache())
						return false;
					if (!TestStatus())
						return false;
					if (!CreateHosts())
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
			bool ConnectDatabase()
			{
				wprintf(L"Connecting to database...\r\n");

				try
				{
					Store.Connect(std::wstring(
						L"Driver={SQL Server};Server=.\\testdb;Database=traphandler;Trusted_Connection = Yes;"));
				}
				catch (std::exception& e)
				{
					dc(L"STORE", L"Exception connecting to database: %S\r\n", e.what());
					return false;
				}
				
				return true;
			}
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
				wprintf(L"Creating 1000 hosts...\r\n");
				for (int i = 0; i < 1000; i++)
				{
					traphandler::model::Host NewHost;
					NewHost.architecture = L"64";
					wchar_t name[100];
					swprintf(name, L"host%i", i);
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
			traphandler::model::odbcStore Store;
			traphandler::model::TrapHandlerModel Model;
		};
	}
}