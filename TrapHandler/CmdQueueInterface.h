#pragma once

#include <string>

namespace traphandler
{
	class CmdQueueInterface
	{
	public:
		// Schedules a command for execution
		virtual bool QueueCommand(const std::wstring &cmd) = 0;
		// checks the queue for completed commands, or command that have timed out
		virtual void ProcessQueue() = 0;

		virtual bool isEmpty() const = 0;

		virtual void Shutdown() = 0;
	};
}