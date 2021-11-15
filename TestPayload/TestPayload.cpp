#include <iostream>

#include <Windows.h>

HMODULE DLL_MODULE_HANDLE = nullptr;

BOOL WINAPI DllMain(
	HINSTANCE hInstDll,
	DWORD fdwReason,
	LPVOID lpReserved
)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DLL_MODULE_HANDLE = hInstDll;
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

void selfDetach()
{
	if (!DLL_MODULE_HANDLE)
		return;
	FreeLibraryAndExitThread(DLL_MODULE_HANDLE, 0);
}

extern "C" __declspec(dllexport) uint32_t connectToHost(void* param)
{
	std::cout << (const char*)param << std::endl;
	selfDetach();
	return 0;
}