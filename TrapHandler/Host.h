#pragma once

#include "Status.h"

namespace traphandler
{
	namespace model
	{
class Host
{
public:
	int id;
	std::wstring hostname;
	std::wstring ip;
	std::wstring ugmonVersion;
	std::wstring versionString;
	int osVersionMajor;
	int osVersionMinor;
	std::wstring osVersionBuild;
	int servicePack;
	std::wstring architecture;
	std::wstring proxyHostName;
	std::wstring platform;
	Status status;
	uint64_t statusChangeTime;
};

	}
}