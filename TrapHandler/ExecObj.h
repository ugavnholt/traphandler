#ifndef _EXECOBJ_HEADER
#define _EXECOBJ_HEADER
#include "stdafx.h"
#include <windows.h>
#include <wchar.h>

/////////////////////////////////////////////////////////////
// Class to spawn an external command, using CreateProcess
// Input and output is redirected, and utility classes are added
// to be able to retrieve status of the executing command
class ExecObj
{
private:
	int LastError;
	int	ReturnValue;
	HANDLE hStdInRd, hStdInWr, hStdOutRd, hStdOutWr, hSaveStdOut;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	SECURITY_ATTRIBUTES saAttr;
	wchar_t *wcsCmd;
public:
	
    // returns a handle that can be used to write to the STDIN on the new process
	inline HANDLE GetStdinWrHandle() { return hStdInWr; }
    // Close the handle returned from GetStdinWrhandle()
    // if handle is closed manually, remember to set the value of the handle
    // to INVALID_HANDLE_VALUE
	inline void CloseStdinWrHandle() { CloseHandle(hStdInWr); hStdInWr = INVALID_HANDLE_VALUE; }
    // Returns a handle to read the STDOUT from the new process
	inline HANDLE GetStdoutRdHandle() { return hStdOutRd; }
    // Returns a handle to the new process
	inline HANDLE GetProcessHandle() { return pi.hProcess; }
    // Returns the Thread ID to the new process, good for PostThreadMessage()
	inline DWORD GetThreadId() { return pi.dwThreadId; }
    // Get the process identifier of the new process
	inline DWORD GetProcessId() { return pi.dwProcessId; }

	///////////////////////////////////////////////////////
    // Method to spawn a new process, as defined in cmd string
    // returns 0 if successfull, otherwize non-0
    int ExecObj::RunCmd(const wchar_t *cmd)
    {
	    if(cmd != NULL)
	    {
		    if(wcsCmd != NULL)
			    delete [] wcsCmd;
		    size_t strLen = wcslen(cmd)+1;
		    wcsCmd = new wchar_t[strLen];
		    memcpy(wcsCmd, cmd, strLen*sizeof(wchar_t));
	    }
	    // Initialize security attributes, to make handle inheritable
	    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	    saAttr.bInheritHandle = TRUE; 
	    saAttr.lpSecurityDescriptor = NULL;

	    // Create the pipe to handle the new process input
	    if(!CreatePipe(&hStdOutRd, &hStdOutWr, &saAttr, 0))
	    {
		    LastError = 1; // Error creating output pipe
		    return 1;
	    }

	    // the read end of the childs stdout should not be inheritable
	    SetHandleInformation( hStdOutRd, HANDLE_FLAG_INHERIT, 0);

	    // Create pipe to handle the new process' output
	    if(!CreatePipe(&hStdInRd, &hStdInWr, &saAttr, 0))
	    {
		    LastError = 2; // Error creating input pipe
		    return 2;
	    }

	    // the write end of the childs stdin should not be inherited
	    SetHandleInformation( hStdInWr, HANDLE_FLAG_INHERIT, 0);

	    si.cb = sizeof(si);
	    si.hStdError = hStdOutWr;
	    si.hStdOutput = hStdOutWr;
	    si.hStdInput = hStdInRd;
	    si.dwFlags |= STARTF_USESTDHANDLES;

	    ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

	    if(CreateProcess(NULL, wcsCmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi) == 0)
	    {
		    // SetStdHandle(STD_OUTPUT_HANDLE,hSaveStdOut);
		    LastError = 3; // Error spawning process
		    pi.hProcess = 0;
		    pi.hThread = 0;
		    return 3;
	    }
	
	    return 0;
    }

    // starts command in new thread and returns immidiately
	inline int RunCmd() 
	{
		if(wcsCmd != NULL)
			return (RunCmd(NULL));
		return 3;
	} 

    // returns the command line string used to spawn the process
	inline const wchar_t* GetCmdStr() { return this->wcsCmd; }

    // Sets the command line string, that are used to spawn the process
	inline void setCmdStr(const wchar_t *cmdStr)
	{
		if(wcsCmd != NULL)
			delete [] wcsCmd;
		size_t strLen = wcslen(cmdStr)+1;
		wcsCmd = new wchar_t[strLen];
		memcpy(wcsCmd, cmdStr, strLen*sizeof(wchar_t));
	}

    // Waits for the command to complete for timeout miliseconds
    // If the command doesn't terminate within the timeout period
    // it is forcefully terminated
	DWORD ExecObj::GetCmdReturn(DWORD timeout)
    {
	    DWORD result;
	    result = WaitForSingleObject(pi.hProcess, timeout);

	    if(result == WAIT_TIMEOUT)
	    {
		    //dw(_T("Timeout occured for command: '%s' - terminating\r\n"), wcsCmd);
		    TerminateProcess(pi.hProcess, 0);
		    result = WaitForSingleObject(pi.hProcess, 500);
	    }

	    DWORD retVal;
	    GetExitCodeProcess(pi.hProcess, &retVal);
	    CloseHandle(hStdOutWr);
	    CloseHandle(hStdInRd);
	    hStdOutWr = INVALID_HANDLE_VALUE;
	    hStdInRd = INVALID_HANDLE_VALUE;

	    return 0;
    }

	// Returns STILL_RUNNING if command is still running, otherwise returns command return value
    inline DWORD ExecObj::GetCmdState()
    {
	    DWORD retVal=0;
	    GetExitCodeProcess(pi.hProcess, &retVal);
	    return retVal;
    }

	////////////////////////////////////////////////////////
    // Standard constructor
    ExecObj::ExecObj() :
    hStdInRd(INVALID_HANDLE_VALUE), hStdInWr(INVALID_HANDLE_VALUE), 
    hStdOutRd(INVALID_HANDLE_VALUE), hStdOutWr(INVALID_HANDLE_VALUE),
    ReturnValue(0), LastError(0), wcsCmd(NULL)
    {
	    ZeroMemory( &si, sizeof(si) );
	    ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
    }

	/////////////////////////////////////////////////////////
    // Destructor
    ExecObj::~ExecObj()
    {
	    // Check the status of the command - we need to force it down, if its still
	    // running at this point - the user app should have taken care of this
	    if(pi.hProcess != 0 && !GetCmdState())
	    {
		    //dd(_T("Command: %s, still running in destructor\r\n"), wcsCmd);
		    TerminateProcess(pi.hProcess, 0);
		    WaitForSingleObject(pi.hProcess, INFINITE);
		    CloseHandle(pi.hProcess);
		    pi.hProcess = 0;
	    }
	    else if(pi.hProcess != 0)
	    {
		    CloseHandle(pi.hProcess);
		    pi.hProcess = 0;
	    }

	    if(pi.hThread != 0)
	    {
		    CloseHandle(pi.hThread);
		    pi.hThread = 0;
	    }


	    // After we have ensured that the command has exited, close all handles
	    // if they are still open
	    if(hStdInRd != INVALID_HANDLE_VALUE)
		    CloseHandle(hStdInRd);
	    if(hStdInWr != INVALID_HANDLE_VALUE)
		    CloseHandle(hStdInWr);
	    if(hStdOutRd != INVALID_HANDLE_VALUE)
		    CloseHandle(hStdOutRd);
	    if(hStdOutWr != INVALID_HANDLE_VALUE)
		    CloseHandle(hStdOutWr);

	    if(wcsCmd != NULL)
		    delete [] wcsCmd;
    }
};

#endif