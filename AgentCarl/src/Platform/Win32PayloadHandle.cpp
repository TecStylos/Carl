#include "Platform/Win32PayloadHandle.h"

#include <Psapi.h>
#include <vector>

namespace AgentCarl
{
	Win32PayloadHandle::Win32PayloadHandle(const std::string& payloadPath, ProcessID processID, PayloadID payloadID)
		: PayloadHandle(payloadPath, processID, payloadID)
	{}

	Win32PayloadHandle::~Win32PayloadHandle()
	{
		detach();
	}

	bool Win32PayloadHandle::call(const std::string& funcName, const void* param, uint64_t paramSize)
	{
		auto func = getPayloadFuncAddr(funcName);
		if (!func)
			return false;

		MemoryHelper targetMem;
		if (param)
		{
			if (!(targetMem = MemoryHelper(targetAlloc(paramSize), this)))
				return false;
			if (!targetWrite(*targetMem, param, paramSize))
				return false;
		}

		DWORD threadID;
		Win32HandleHelper hThread = CreateRemoteThread(*m_hProcTarget, NULL, 0, func, *targetMem, 0, &threadID);
		if (!*hThread)
			return false;

		DWORD retVal;
		WaitForSingleObject(*hThread, INFINITE);
		GetExitCodeThread(*hThread, (LPDWORD)&retVal);

		return true;
	}

	bool Win32PayloadHandle::injectImpl()
	{
		uint64_t payloadPathSize = m_payloadPath.size() + 1;

		m_hProcTarget = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_processID);
		if (!*m_hProcTarget)
			return false;

		MemoryHelper nameMem = MemoryHelper(targetAlloc(payloadPathSize), this);
		if (!nameMem)
			return false;

		if (!targetWrite(*nameMem, m_payloadPath.c_str(), payloadPathSize))
			return false;

		HMODULE kernel32 = GetModuleHandle("kernel32");
		if (!kernel32)
			return false;

		auto funcLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(kernel32, "LoadLibraryA");

		Win32HandleHelper hThread = CreateRemoteThread(*m_hProcTarget, NULL, 0, funcLoadLibrary, *nameMem, 0, nullptr);
		if (!*hThread)
			return false;

		DWORD retVal;
		WaitForSingleObject(*hThread, INFINITE);
		GetExitCodeThread(*hThread, (LPDWORD)&retVal);

		m_payloadBase = getPayloadBaseAddr();

		if (!m_payloadBase)
			return false;

		return true;
	}

	bool Win32PayloadHandle::detachImpl()
	{
		// TODO: Implement payload detach.
		return true;
	}

	HMODULE Win32PayloadHandle::getPayloadBaseAddr() const
	{
		bool success = true;
		std::vector<HMODULE> modules(16);
		DWORD nBytesNeeded = 0;

		if (!EnumProcessModulesEx(*m_hProcTarget, modules.data(), (DWORD)modules.size() * sizeof(HMODULE), &nBytesNeeded, LIST_MODULES_ALL))
			return nullptr;
		if (modules.size() * sizeof(HMODULE) < nBytesNeeded)
		{
			modules.resize(nBytesNeeded / sizeof(HMODULE));
			if (!EnumProcessModulesEx(*m_hProcTarget, modules.data(), (DWORD)modules.size() * sizeof(HMODULE), &nBytesNeeded, LIST_MODULES_ALL))
				return nullptr;
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

		return nullptr;
	}

	LPTHREAD_START_ROUTINE Win32PayloadHandle::getPayloadFuncAddr(const std::string& funcName) const
	{
		HMODULE hLoaded = LoadLibraryA(m_payloadPath.c_str());
		if (!hLoaded)
			return nullptr;

		void* lpFunc = GetProcAddress(hLoaded, funcName.c_str());
		uint64_t offset = (char*)lpFunc - (char*)hLoaded;
		return (LPTHREAD_START_ROUTINE)((uint64_t)m_payloadBase + offset);
	}

	void* Win32PayloadHandle::targetAlloc(uint64_t size)
	{
		return VirtualAllocEx(*m_hProcTarget, NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	}
	bool Win32PayloadHandle::targetFree(void* base)
	{
		return VirtualFreeEx(*m_hProcTarget, base, 0, MEM_RELEASE);
	}
	bool Win32PayloadHandle::targetWrite(void* destTarget, const void* src, uint64_t size)
	{
		SIZE_T nWritten = 0;
		return WriteProcessMemory(*m_hProcTarget, destTarget, src, size, &nWritten);
	}
	bool Win32PayloadHandle::targetRead(void* dest, const void* srcTarget, uint64_t size)
	{
		SIZE_T nRead = 0;
		return ReadProcessMemory(*m_hProcTarget, srcTarget, dest, size, &nRead);
	}
}