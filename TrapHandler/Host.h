#pragma once

namespace traphandler
{
	namespace model
	{
class Host
{
public:
	uuid_t id;
	std::wstring hostname;
	std::wstring ip;
	std::wstring ugmonVersion;
	std::wstring versionString;
	int osVersionMajor;
	int osVersionMinor;
	int osVersionBuild;
	int servicePack;
	std::wstring architecture;
	FILETIME ftFirstSeenTime;
	FILETIME ftLastHelloTime;
	std::wstring proxyHostName;
	std::wstring platform;
	int status;
};

	}
}