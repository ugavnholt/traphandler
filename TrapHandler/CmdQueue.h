#pragma once

#include "AgentEvent.h"
#include <concurrent_queue.h>
#include "ExecObj.h"
#include <list>

namespace traphandler
{
	class CmdQueue
	{
	public:
		void QueueCommand(const std::wstring& cmd);
		inline bool isEmpty() const { return cmdQueue.empty(); }
		~CmdQueue();

		void ProcessQueue();
		void ProcessCommands();
		inline size_t GetExecutedCommands() const { return dwExecutedCommands; }
		void Shutdown();
	private:
		concurrency::concurrent_queue<ExecObj*> cmdQueue;
		std::list<ExecObj*> runningCommands;
		size_t nCommands = 0;
		size_t dwExecutedCommands = 0;
	};

	inline void Shutdown()
	{

	}

	inline void CmdQueue::ProcessQueue()
	{
		ExecObj *newCmd;
		while ((nCommands < MAXCONCURRENTCMDS) && cmdQueue.try_pop(newCmd))
		{
			// wprintf(L"running command: %s\r\n", newCmd->GetCmdStr());
			if (newCmd->RunCmd() != NULL)
			{
				dw(Facility, L"Error launching command: %s\r\n", newCmd->GetCmdStr());
				delete newCmd;
			}
			else
			{
				runningCommands.push_back(newCmd);
				nCommands++;
			}
		}
	}

	inline CmdQueue::~CmdQueue()
	{
		dw(Facility, L"Emptying command queue for %u outstanding command request(s), due to application shutdown\r\n", cmdQueue.unsafe_size());
		ExecObj *tmpCmd;
		while (cmdQueue.try_pop(tmpCmd))
		{
			delete tmpCmd;
		}

		// Terminate any running commands
		if (nCommands > 0)
		{
			dw(Facility, L"Terminating %u command(s) that are still running, due to application shutdown\r\n", nCommands);
			while (!runningCommands.empty())
			{
				runningCommands.front()->GetCmdReturn(1000);
				delete runningCommands.front();
				runningCommands.pop_front();
				dwExecutedCommands++;
			}
		}
	}

	inline void CmdQueue::ProcessCommands()
	{
		///////////////////////////////
		// Check for any completed commands
		auto cmdIt = runningCommands.begin();
		while (cmdIt != runningCommands.end())
		{
			// have the command completed
			DWORD cmdRet = (*cmdIt)->GetCmdState();
			if (cmdRet != STILL_ACTIVE)	// command completed
			{
				if (cmdRet != 0)
				{
					dmi(L"CMDQUEUE", L"Command: %s returned %x\r\n", (*cmdIt)->GetCmdStr(), cmdRet);
				}
				else
				{
					dd(L"CMDQUEUE", L"Command: %s completed\r\n", (*cmdIt)->GetCmdStr());
				}
				delete (*cmdIt);
				cmdIt = runningCommands.erase(cmdIt);
				nCommands--;	// release a command slot
				dwExecutedCommands++;
			}
			else
				cmdIt++;
		}
	}

	inline void CmdQueue::QueueCommand(const std::wstring& cmd)
	{
		if (!cmd.empty())
		{
			ExecObj *newCmd = new ExecObj();
			newCmd->setCmdStr(cmd.c_str());
			cmdQueue.push(newCmd);
			dd(L"CMDQUEUE", L"Queued command: %s\r\n", cmd.c_str());
		}
	}
}