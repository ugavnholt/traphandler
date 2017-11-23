#pragma once

#include "utils.h"
#include "TimerObj.h"
#include <unordered_map>

namespace traphandler
{
class TimerService
{
public:
	~TimerService()
	{
		auto timIt = timers.begin();
		while (timIt != timers.end())
		{
			//delete (*timIt);
			timIt = timers.erase(timIt);
		}
	}
	// Given a hostname and a platform id, this method locates and updates a given timer
	// if the time existed prior to this call, true is returned, otherwise false is returned
	// to indicate that a new timer was created
	bool TouchTimer(const std::wstring& hostname, 
		const std::wstring& platformId, 
		int durationSecs, 
		model::timerType type = model::timerType::heartBeatExpired)
	{
		FILETIME ftNow;
		GetSystemTimeAsFileTime(&ftNow);
		uint64_t now = utils::fttoull(ftNow);
	}

/*	inline TimerObj* LocateTimer(const wchar_t *id, const timerType type)
	{
		std::list<TimerObj*>::iterator it;
		for (it = ActiveTimers.begin(); it != ActiveTimers.end(); it++)
		{
			if ((*it)->GetTimerType() == type && wcsicmp((*it)->GetTimerID(), id) == 0)
				return (*it);
		}
		return NULL;
	}

	inline void DeleteTimer(const wchar_t *id, const timerType type)
	{
		auto it = ActiveTimers.begin();
		while (it != ActiveTimers.end())
		{
			if ((*it)->GetTimerType() == type && wcsicmp((*it)->GetTimerID(), id) == 0)
			{
				delete *it;
				it = ActiveTimers.erase(it);
				return;
			}
			it++;
		}
	}*/
private:
	std::unordered_map<std::wstring, model::TimerObj> timers;
}; // class TimerService
} // namespace traphandler