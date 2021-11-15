#include "Platform/Win32PayloadHandle.h"

#include <Psapi.h>
#include <vector>

#include "PayloadError.h"
#include "Platform/Win32ErrorMessage.h"

namespace AgentCarl
{
	Win32PayloadHandle::Win32PayloadHandle(const std::string& payloadPath, ProcessID processID)
		: PayloadHandle(payloadPath, processID)
	{}

	void Win32PayloadHandle::call(const std::string& funcName, const void* param, uint64_t paramSize)
	{
		auto func = getPayloadFuncAddr(funcName);

		MemoryHelper targetMem;
		if (param)
		{
			targetMem = MemoryHelper(targetAlloc(paramSize), this);
			targetWrite(*targetMem, param, paramSize);
		}

		DWORD threadID;
		Win32HandleHelper hThread = CreateRemoteThread(*m_hProcTarget, NULL, 0, func, *targetMem, 0, &threadID);
		if (!*hThread)
			throw PayloadError("Unable to create payload thread!: " + Win32LastErrorMessage());

		DWORD retVal;
		WaitForSingleObject(*hThread, INFINITE);
		GetExitCodeThread(*hThread, (LPDWORD)&retVal);
	}

	void Win32PayloadHandle::injectImpl()
	{
		uint64_t payloadPathSize = m_payloadPath.size() + 1;

		m_hProcTarget = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, m_processID);
		if (!*m_hProcTarget)
			throw PayloadError("Unable to open process!: " + Win32LastErrorMessage());

		MemoryHelper nameMem = MemoryHelper(targetAlloc(payloadPathSize), this);

		targetWrite(*nameMem, m_payloadPath.c_str(), payloadPathSize);

		HMODULE kernel32 = GetModuleHandle("kernel32");
		if (!kernel32)
			throw PayloadError("Unable to get 'kernel32' handle!: " + Win32LastErrorMessage());

		auto funcLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryA");

		Win32HandleHelper hThread = CreateRemoteThread(*m_hProcTarget, NULL, 0, funcLoadLibrary, *nameMem, 0, nullptr);
		if (!*hThread)
			throw PayloadError("Unable to create payload thread!: " + Win32LastErrorMessage());

		DWORD retVal;
		WaitForSingleObject(*hThread, INFINITE);
		GetExitCodeThread(*hThread, (LPDWORD)&retVal);

		m_payloadBase = getPayloadBaseAddr();
	}

	HMODULE Win32PayloadHandle::getPayloadBaseAddr() const
	{
		bool success = true;
		std::vector<HMODULE> modules(16);
		DWORD nBytesNeeded = 0;

		if (!EnumProcessModulesEx(*m_hProcTarget, modules.data(), (DWORD)modules.size() * sizeof(HMODULE), &nBytesNeeded, LIST_MODULES_ALL))
			throw PayloadError("Unable to enumerate process modules!: " + Win32LastErrorMessage());
		if (modules.size() * sizeof(HMODULE) < nBytesNeeded)
		{
			modules.resize(nBytesNeeded / sizeof(HMODULE));
			if (!EnumProcessModulesEx(*m_hProcTarget, modules.data(), (DWORD)modules.size() * sizeof(HMODULE), &nBytesNeeded, LIST_MODULES_ALL))
				throw PayloadError("Unable to enumerate process modules!: " + Win32LastErrorMessage());
		}

		DWORD nModules = nBytesNeeded / sizeof(HMODULE);

		for (DWORD i = 0; i < nModules; ++i)
		{
			auto hModule = modules[i];
			char nameBuff[1024];
			if (!GetModuleFileNameEx(*m_hProcTarget, hModule, nameBuff, sizeof(nameBuff)))
				continue;

			if (nameBuff == m_payloadPath)
				return hModule;
		}

		throw PayloadError("Unable to find payload in the process modules!");
	}

	LPTHREAD_START_ROUTINE Win32PayloadHandle::getPayloadFuncAddr(const std::string& funcName) const
	{
		HMODULE hLoaded = LoadLibraryA(m_payloadPath.c_str());
		if (!hLoaded)
			throw PayloadError("Unable to load the payload!: " + Win32LastErrorMessage());

		void* lpFunc = GetProcAddress(hLoaded, funcName.c_str());
		if (!lpFunc)
			throw PayloadError("Unable to get the requested function from the payload!: " + Win32LastErrorMessage());
		uint64_t offset = (char*)lpFunc - (char*)hLoaded;
		return (LPTHREAD_START_ROUTINE)((uint64_t)m_payloadBase + offset);
	}

	void* Win32PayloadHandle::targetAlloc(uint64_t size)
	{
		void* result;
		if (!(result = VirtualAllocEx(*m_hProcTarget, NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE)))
			throw PayloadError("Unable to allocate memory in the target process!: " + Win32LastErrorMessage());
		return result;
	}
	void Win32PayloadHandle::targetFree(void* base)
	{
		if (!VirtualFreeEx(*m_hProcTarget, base, 0, MEM_RELEASE))
			throw PayloadError("Unable to free memory in the target process!: " + Win32LastErrorMessage());
	}
	void Win32PayloadHandle::targetWrite(void* destTarget, const void* src, uint64_t size)
	{
		SIZE_T nWritten = 0;
		if (!WriteProcessMemory(*m_hProcTarget, destTarget, src, size, &nWritten))
			throw PayloadError("Unable to write to memory in the target process!: " + Win32LastErrorMessage());
	}
	void Win32PayloadHandle::targetRead(void* dest, const void* srcTarget, uint64_t size)
	{
		SIZE_T nRead = 0;
		if (!ReadProcessMemory(*m_hProcTarget, srcTarget, dest, size, &nRead))
			throw PayloadError("Unable to read from memory in the target process!: " + Win32LastErrorMessage());
	}
}