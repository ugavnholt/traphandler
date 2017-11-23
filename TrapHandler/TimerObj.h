#pragma once

namespace traphandler
{
	namespace model
	{


enum timerType { maintExpired = 0, heartBeatExpired = 1 };

class TimerObj
{
private:
	uint64_t		startTime;
	uint64_t		createTime;
	uint64_t		duration;
	unsigned int	uHits;
	std::wstring	timerId;
	timerType		type;
public:
	inline int64_t GetDuration() const { return duration; }
	TimerObj(uint64_t StartTime, int durationSecs, const std::wstring& id, const timerType Type) 
		: type(Type), uHits(0)
	{
		duration = durationSecs;
		duration = duration*FTCLICKSQERSEC;
		FILETIME ftNow;
		GetSystemTimeAsFileTime(&ftNow);
		startTime = StartTime;
		timerId = id;
	}
	~TimerObj()
	{
	}

	inline bool isExpired(uint64_t now)
	{
		if(now - startTime > duration)
			return true;	// timer is expired
		else
			return false;	// timer has not expired...yet
	}

	const std::wstring GetTimerID() { return timerId; }
	const timerType GetTimerType() { return type; }

	inline void Update(uint64_t StartTime, const int durationSecs) 
	{
		duration = durationSecs;
		duration = duration *FTCLICKSQERSEC; 
		uHits++; 
		startTime = StartTime;
	}
};

	} // namespace model
} // namespace traphandler