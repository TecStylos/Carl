
#include <EHSN.h>
#include <iostream>
#include <Windows.h>
#include <string>
#include <thread>

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
	FreeLibrary(DLL_MODULE_HANDLE);
}

void mainFunc(std::string port)
{
	EHSN::net::ManagedSocket queue(std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0));

	uint8_t connectCount = 0;
	while (!queue.getSock()->isConnected() && connectCount++ < 8)
		queue.connect("localhost", port, true);

	if (!queue.getSock()->isConnected())
		selfDetach();

	std::cout << "Connected to host!" << std::endl;

	selfDetach();
}

extern "C" __declspec(dllexport) uint32_t connectToHost(void* param)
{
	std::thread thread(mainFunc, (const char*)param);
	thread.detach();
	return 0;
}