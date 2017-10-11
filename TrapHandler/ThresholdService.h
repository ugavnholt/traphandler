#pragma once

#include <list>
#include "Threshold.h"
#include "nanodbc.h"

namespace traphandler
{
	namespace model
	{
		// Given a database connection, the threshold service is responsible
		// for the collection, caching and evaluation of thresholds
		class ThresholdService
		{
		public:
			ThresholdService() {}
			Threshold &GetThresholdForFS(std::wstring &hostname, std::wstring &volume_name, int64_t fsSizeMb)
			{
				for (auto it : threshold_cache)
				{
					if (it.Matches(hostname, volume_name, fsSizeMb))
						return it;
				}
			}
			void LoadCache(nanodbc::connection &connection)
			{
				threshold_cache.clear();
				nanodbc::result row = nanodbc::execute(connection,
					L"SELECT "
					L"TreshFreeMBWarn,"		// 0 (LONG)
					L"ThreshFreeMBHigh,"	// 1 (LONG)
					L"ThreshUtilWarn,"		// 2 (DOUBLE)
					L"ThreshUtilHigh,"		// 3 (DOUBLE)
					L"WarnSev,"				// 4 (STR)
					L"highSev,"				// 5 (STR)
					L"HostExpression,"		// 6 (STR)
					L"VolumeExpression,"	// 7 (STR)
					L"MinFSSizeMB,"			// 8 (LONG)
					L"MaxFSSizeMB "			// 9 (LONG)
					L"FROM dbo.FSThresholds ORDER BY EvalOrder");
				for (int i=1; row.next(); ++i)
				{
					long FreeMBWarn = 0;
					row.get_ref<long>(0, FreeMBWarn);
					long FreeMBHigh = 0;
					row.get_ref<long>(1, FreeMBHigh);
					double UtilWarn = 0.0f;
					row.get_ref<double>(2, UtilWarn);
					double UtilHigh = 0.0f;
					row.get_ref<double>(3, UtilHigh);
					std::string warnSeverity;
					row.get_ref<std::string>(4, warnSeverity);
					std::string highSeverity;
					row.get_ref<std::string>(5, highSeverity);
					std::string hostExpr;
					row.get_ref<std::string>(6, hostExpr);
					std::string volExpr;
					row.get_ref<std::string>(7, volExpr);
					long MinFSSize = 0;
					row.get_ref<long>(8, MinFSSize);
					long MaxFSSize = 0;
					row.get_ref<long>(9, MaxFSSize);
					threshold_cache.emplace_back(
						FreeMBWarn, 
						FreeMBHigh, 
						UtilWarn, 
						UtilHigh, 
						warnSeverity, 
						highSeverity, 
						hostExpr, 
						volExpr, 
						MinFSSize, 
						MaxFSSize
					);
				}
			}
			std::list<Threshold> threshold_cache;
		private:
		};

	} // namespace model
} // namespace traphandler