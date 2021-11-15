#include "Carl/Platform/Win32AgentRunner.h"

#include <Windows.h>
#include <string>

#include "Carl/CarlError.h"
#include "Carl/Platform/Win32ErrorMessage.h"

namespace Carl
{
	void Win32RunAgent(const std::string& agentExe, const std::string& cmdLine)
	{
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		si.cb = sizeof(si);

		if (!CreateProcessA(
			(char*)agentExe.c_str(),
			(char*)cmdLine.c_str(),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&si,
			&pi
		))
			throw CarlError("Unable to start AgentCarl!: " + Win32LastErrorMessage());

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD agentErr;
		if (!GetExitCodeProcess(pi.hProcess, &agentErr))
			throw CarlError("AgentCarl did not run as expected!: " + Win32LastErrorMessage());
		if (agentErr != 0)
			throw CarlError("AgentCarl did not run as expected!: Exitcode: " + std::to_string((int)agentErr));

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}