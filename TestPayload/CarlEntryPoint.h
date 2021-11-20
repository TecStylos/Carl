#pragma once

#include <Windows.h>

namespace Carl
{
	HMODULE DLLModuleHandle = nullptr;

	void selfDetach()
	{
		if (!DLLModuleHandle)
			return;

		FreeLibraryAndExitThread(DLLModuleHandle, 0);
	}

	// To be defined by every payload
	extern bool CreateConsole;
	extern int MainFunc(void* param);
}

BOOL WINAPI DllMain(
	HINSTANCE hInstDll,
	DWORD fdwReason,
	LPVOID lpReserved
)
{
	if (DetourIsHelperProcess())
		return TRUE;

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		Carl::DLLModuleHandle = hInstDll;
		DisableThreadLibraryCalls(hInstDll);
		DetourRestoreAfterWith();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

extern "C" DWORD WINAPI CarlMainFuncEntryPoint(void* param)
{
	bool allocatedConsole = false;
	if (Carl::CreateConsole && AllocConsole())
	{
		allocatedConsole = true;
		FILE* temp;
		freopen_s(&temp, "CONOUT$", "w", stdout);
		freopen_s(&temp, "CONIN$", "r", stdin);
	}

	int result = Carl::MainFunc(param);

	if (allocatedConsole)
		FreeConsole();

	Carl::selfDetach();
	return result;
}

extern "C" __declspec(dllexport) DWORD WINAPI connectToHost(void* param)
{
	#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
	CreateThread(NULL, 0, CarlMainFuncEntryPoint, param, 0, NULL);
	return 0;
}