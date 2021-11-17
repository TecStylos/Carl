#include <CarlPayload.h>

#include <iostream>
#include <Windows.h>
#include <string>
#include <mmsystem.h>
#include <dsound.h>

#include <detours.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

HMODULE DLL_MODULE_HANDLE = nullptr;
LONG TRANS_COMMIT = 0;

class MySoundBuffer8 : public IDirectSoundBuffer8
{
public:
	HRESULT __stdcall myLock(
		DWORD dwOffset,
		DWORD dwBytes,
		LPVOID* ppvAudioPtr1,
		LPDWORD pdwAudioBytes1,
		LPVOID* ppvAudioPtr2,
		LPDWORD pdwAudioBytes2,
		DWORD dwFlags
	)
	{
		std::cout << "Params:" << std::endl
			<< "  " << "dwOffset       " << dwOffset << std::endl
			<< "  " << "dwBytes        " << dwBytes << std::endl
			<< "  " << "ppvAudioPtr1   " << ppvAudioPtr1 << std::endl
			<< "  " << "pdwAudioBytes1 " << pdwAudioBytes1 << std::endl
			<< "  " << "ppvAudioPtr2   " << ppvAudioPtr2 << std::endl
			<< "  " << "pdwAudioBytes2 " << pdwAudioBytes2 << std::endl
			<< "  " << "dwFlags        " << dwFlags << std::endl;
		return (this->*realLock)(
			dwOffset,
			dwBytes,
			ppvAudioPtr1,
			pdwAudioBytes1,
			ppvAudioPtr2,
			pdwAudioBytes2,
			dwFlags
			);
	}
public:
	inline static HRESULT (__stdcall MySoundBuffer8::* realLock)(
		DWORD dwOffset,
		DWORD dwBytes,
		LPVOID* ppvAudioPtr1,
		LPDWORD pdwAudioBytes1,
		LPVOID* ppvAudioPtr2,
		LPDWORD pdwAudioBytes2,
		DWORD dwFlags
		) = 0;
};


typedef HRESULT(*DirectSoundCreate8_t)(
	LPCGUID, LPDIRECTSOUND8*, LPUNKNOWN
	);
static DirectSoundCreate8_t RealDirectSoundCreate8 = 0;

HRESULT MyDirectSoundCreate8(
	LPCGUID lpcGuidDevice,
	LPDIRECTSOUND8* ppDS8,
	LPUNKNOWN pUnkOuter
)
{
	std::cout << "Called MyDirectSoundCreate8 :)" << std::endl;
	return RealDirectSoundCreate8(
		lpcGuidDevice,
		ppDS8,
		pUnkOuter
	);
}


BOOL WINAPI DllMain(
	HINSTANCE hInstDll,
	DWORD fdwReason,
	LPVOID lpReserved
)
{
	if (DetourIsHelperProcess())
		return TRUE;

	/*static HRESULT (__stdcall MySoundBuffer8:: * pfMyLock)(
		DWORD dwOffset,
		DWORD dwBytes,
		LPVOID * ppvAudioPtr1,
		LPDWORD pdwAudioBytes1,
		LPVOID * ppvAudioPtr2,
		LPDWORD pdwAudioBytes2,
		DWORD dwFlags
		) = &MySoundBuffer8::myLock;*/

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DLL_MODULE_HANDLE = hInstDll;
		DisableThreadLibraryCalls(hInstDll);
		DetourRestoreAfterWith();

		{
			HMODULE dsound = GetModuleHandleA("dsound.dll");
			if (dsound)
				RealDirectSoundCreate8 = (DirectSoundCreate8_t)GetProcAddress(dsound, "DirectSoundCreate8");
		}

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		//DetourAttach(&(PVOID&)MySoundBuffer8::realLock, *(PBYTE*)&pfMyLock);
		DetourAttach(&(PVOID&)RealDirectSoundCreate8, MyDirectSoundCreate8);
		TRANS_COMMIT = DetourTransactionCommit();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		//DetourDetach(&(PVOID&)MySoundBuffer8::realLock, *(PBYTE*)&pfMyLock);
		DetourDetach(&(PVOID&)RealDirectSoundCreate8, MyDirectSoundCreate8);
		DetourTransactionCommit();
		break;
	}

	return TRUE;
}

void* getAddrFromVTable(uint64_t index)
{
	CoInitialize(NULL);

	IDirectSound8* pDirectSound8 = nullptr;
	DirectSoundCreate8(&DSDEVID_DefaultPlayback, &pDirectSound8, NULL);

	SetConsoleTitle("ABCDEFGH");
	Sleep(40);
	HWND hWnd = FindWindow(NULL, "ABCDEFGH");

	pDirectSound8->SetCooperativeLevel(hWnd, DSSCL_NORMAL);

	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 8192;
	format.wBitsPerSample = 16;
	format.nBlockAlign = (format.nChannels * format.wBitsPerSample) / 8;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	format.cbSize = 0;

	DSBUFFERDESC desc;
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_CTRLVOLUME;
	desc.dwBufferBytes = 1024;
	desc.dwReserved = 0;
	desc.lpwfxFormat = &format;
	desc.guid3DAlgorithm = DS3DALG_DEFAULT;
	IDirectSoundBuffer* pBuffer;
	HRESULT res = pDirectSound8->CreateSoundBuffer(&desc, &pBuffer, NULL);

	IDirectSoundBuffer8* pBuffer8;
	pBuffer->QueryInterface(IID_IDirectSoundBuffer8, (void**)&pBuffer8);
	pBuffer->Release();

	uintptr_t* vtable = *(uintptr_t**)pBuffer8;
	void* retAddr = (void*)vtable[index];

	pBuffer8->Release();

	return retAddr;
}

void makeDirectSoundBuffer8LockDetour()
{
	// vtable[11]

	void* realLock = getAddrFromVTable(11);
	MySoundBuffer8::realLock = (HRESULT(__cdecl MySoundBuffer8::*&)(DWORD, DWORD, LPVOID*, LPDWORD, LPVOID*, LPDWORD, DWORD))realLock;

	static HRESULT(__stdcall MySoundBuffer8::* pfMyLock)(
		DWORD dwOffset,
		DWORD dwBytes,
		LPVOID * ppvAudioPtr1,
		LPDWORD pdwAudioBytes1,
		LPVOID * ppvAudioPtr2,
		LPDWORD pdwAudioBytes2,
		DWORD dwFlags
		) = &MySoundBuffer8::myLock;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)MySoundBuffer8::realLock, *(PBYTE*)&pfMyLock);
	DetourTransactionCommit();
}

void removeDirectSoundBuffer8LockDetour()
{
	static HRESULT(__stdcall MySoundBuffer8:: * pfMyLock)(
		DWORD dwOffset,
		DWORD dwBytes,
		LPVOID * ppvAudioPtr1,
		LPDWORD pdwAudioBytes1,
		LPVOID * ppvAudioPtr2,
		LPDWORD pdwAudioBytes2,
		DWORD dwFlags
		) = &MySoundBuffer8::myLock;

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)MySoundBuffer8::realLock, *(PBYTE*)&pfMyLock);
	DetourTransactionCommit();
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
	makeDirectSoundBuffer8LockDetour();

	std::cout << "Transaction commit: " << TRANS_COMMIT << std::endl;

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

	removeDirectSoundBuffer8LockDetour();
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