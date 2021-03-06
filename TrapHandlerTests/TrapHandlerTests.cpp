// TrapHandlerTests.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <vld.h>
#include "../TrapHandler/TraceEngine.cpp"
#include "ModelTests.h"

int wmain(int argc, wchar_t* argv[])
{
	// Model also initializes logging
	traphandler::tests::ModelTests modelTests;
	wprintf(L"Running model tests...\r\n");
	if (!modelTests.RunTests())
	{
		wprintf(L"FAILED: Model Tests\r\n");
	}
	else
	{
		wprintf(L"SUCCESS: Model Tests\r\n");
	}
	delete pTrace; 
	pTrace = nullptr;
	_getwch();
    return 0;
}

