#include "stdafx.h"

enum timerType { maintExpired = 0, heartBeatExpired = 1 };

inline ULONGLONG fttoull(const FILETIME &ft)
{
	ULONGLONG retVal = ft.dwHighDateTime;
	retVal = retVal << 32;
	return  (retVal += ft.dwLowDateTime);
}

class TimerObj
{
private:
	FILETIME		ftStartTime;
	FILETIME		ftCreateTime;
	ULONGLONG		duration;
	unsigned int	uHits;
	wchar_t			*TimerID;
	timerType		type;
public:
	inline ULONGLONG GetDuration() { return duration; }
	TimerObj(FILETIME *StartTime, int durationSecs, const wchar_t *id, const timerType Type) 
		: type(Type), uHits(0)
	{
		duration = durationSecs;
		duration = duration*FTCLICKSQERSEC;
		GetSystemTimeAsFileTime(&ftCreateTime);
		memcpy(&ftStartTime, StartTime, sizeof(FILETIME));
		TimerID = new wchar_t[wcslen(id)+1];
		memcpy(TimerID, id, (wcslen(id)+1) * sizeof(wchar_t));
	}
	~TimerObj()
	{
		delete [] TimerID;
	}

	inline bool isExpired(FILETIME *now)
	{
		if(fttoull(*now) - fttoull(ftStartTime) > duration)
			return true;	// timer is expired
		else
			return false;	// timer has not expired...yet
	}

	const wchar_t* GetTimerID() { return TimerID; }
	const timerType GetTimerType() { return type; }

	inline void Update(const FILETIME *ftStart, const int durationSecs) 
	{ 
		duration = durationSecs;
		duration = duration *FTCLICKSQERSEC; 
		uHits++; 
		memcpy(&ftStartTime, ftStart, sizeof(FILETIME));
	}
};
