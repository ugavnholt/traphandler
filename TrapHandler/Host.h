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
	int osVersionMajor = 0;
	int osVersionMinor = 0;
	std::wstring osVersionBuild;
	int servicePack = 0;
	std::wstring architecture;
	std::wstring proxyHostName;
	std::wstring platform;
	Status status;
	uint64_t statusChangeTime = 0;

	Host()
	{
	}

	Host(const Host& other)
	{
		*this = other;
	}

	const std::wstring to_wstring()
	{
		std::wstring str = hostname;
		str += L"(";
		str += std::to_wstring(id);
		str += L")";
		return str;
	}

	

	Host & operator=(const Host& other)
	{
		id = other.id;
		hostname = other.hostname;
		ip = other.ip;
		ugmonVersion = other.ugmonVersion;
		versionString = other.versionString;
		osVersionMajor = other.osVersionMajor;
		osVersionMinor = other.osVersionMinor;
		osVersionBuild = other.osVersionBuild;
		servicePack = other.servicePack;
		architecture = other.architecture;
		proxyHostName = other.proxyHostName;
		platform = other.platform;
		status.status = other.status.status;
		statusChangeTime = other.statusChangeTime;
		return *this;
	}
};

	}
}