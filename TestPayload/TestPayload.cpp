#include <CarlPayload.h>

#include <iostream>
#include <Windows.h>
#include <string>

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

	std::cout << "Freeing library..." << std::endl;

	FreeLibraryAndExitThread(DLL_MODULE_HANDLE, 0);
}

void mainFunc(std::string port)
{
	auto sock = std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0);
	sock->connect("localhost", port, true);

	/*EHSN::net::ManagedSocket queue(std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0));

	uint8_t connectCount = 0;
	while (!queue.isConnected() && connectCount++ < 8)
		queue.connect("localhost", port, true);

	if (!queue.isConnected())
		return;

	while (queue.isConnected())
	{
		auto pack = queue.pull(Carl::PT_ECHO_REQUEST);
		if (!pack.buffer)
			break;
		std::string line = (char*)pack.buffer->data();
		std::cout << line << std::endl;
	}*/

	std::cout << "Starting shutdown..." << std::endl;
}

extern "C" DWORD WINAPI mainFuncThread(void* param)
{
	std::string paramStr = (char*)param;
	mainFunc(paramStr);
	selfDetach();
	return 0;
}

extern "C" __declspec(dllexport) DWORD WINAPI connectToHost(void* param)
{
	CreateThread(NULL, 0, mainFuncThread, param, 0, NULL);
	return 0;
}