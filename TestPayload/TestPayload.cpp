#include <CarlPayload.h>

#include <iostream>
#include <Windows.h>
#include <string>

#include <detours.h>

#include "MyAudioClient.h"

HMODULE DLL_MODULE_HANDLE = nullptr;
LONG TRANS_COMMIT = 0;


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
		DLL_MODULE_HANDLE = hInstDll;
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

void selfDetach()
{
	if (!DLL_MODULE_HANDLE)
		return;

	FreeLibraryAndExitThread(DLL_MODULE_HANDLE, 0);
}

void mainFunc(std::string port)
{
	makeAudioRenderClientDetours();

	EHSN::net::ManagedSocket queue(std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0));

	uint8_t connectCount = 0;
	while (!queue.isConnected() && connectCount++ < 8)
		queue.connect("localhost", port, true);

	if (!queue.isConnected())
		return;

	std::cout << "Connected to host!" << std::endl;

	while (queue.isConnected())
	{
		auto pack = queue.pull(Carl::PT_ECHO_REQUEST);
		if (!pack.buffer)
			break;
		std::string line = (char*)pack.buffer->data();
		std::cout << line << std::endl;
	}

	removeAudioRenderClientDetours();
}

extern "C" DWORD WINAPI mainFuncThread(void* param)
{
	bool allocatedConsole = false;
	if (AllocConsole())
	{
		allocatedConsole = true;
		FILE *temp;
		freopen_s(&temp, "CONOUT$", "w", stdout);
		freopen_s(&temp, "CONIN$", "r", stdin);
	}

	std::string paramStr = (char*)param;
	mainFunc(paramStr);

	if (allocatedConsole)
	{
		FreeConsole();
	}

	selfDetach();
	return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI connectToHost(void* param)
{
	#pragma comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__)
	CreateThread(NULL, 0, mainFuncThread, param, 0, NULL);
	return 0;
}