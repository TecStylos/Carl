#include "Carl/Platform/Win32ProcessArchitecture.h"

#include <Windows.h>

#include "Carl/CarlError.h"

namespace Carl
{
	ProcArch Win32GetProcArch(ProcessID pid)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (!hProcess)
			throw CarlError("Cannot open process to detect architecture!");

		BOOL isWow64;
		if (!IsWow64Process(hProcess, &isWow64))
			throw CarlError("Cannot retrieve architecture from process!");

		if (isWow64)
			return ProcArch::x86;

		SYSTEM_INFO si = { 0 };
		GetSystemInfo(&si);
		return (
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64
			) ?
			ProcArch::x64 : ProcArch::x86;
	}
}