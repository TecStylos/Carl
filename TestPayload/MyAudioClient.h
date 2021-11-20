#pragma once

#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <mutex>

#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <detours.h>

#include "GetFuncFromObjVTable.h"

#if !defined(PAYLOAD_ARCH_x86) && !defined(PAYLOAD_ARCH_x64)
#define PAYLOAD_ARCH_UNKNOWN
#endif


const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);

class MyAudioClient;
class MyAudioRenderClient;

typedef HRESULT(__stdcall MyAudioClient::* MyAudioClient_GetCurrentPadding_t)(UINT32*);
typedef HRESULT(__stdcall MyAudioRenderClient::* MyAudioRenderClient_GetBuffer_t)(UINT32, BYTE**);
typedef HRESULT(__stdcall MyAudioRenderClient::* MyAudioRenderClient_ReleaseBuffer_t)(UINT32, DWORD);

std::mutex gMtxAudioClients;
std::mutex gMtxRenderClients;
static std::map<IAudioClient*, WAVEFORMATEX*> gAudioClients;
static std::set<IAudioRenderClient*> gRenderClients;

#include <fstream>
std::ofstream outFileStream;


class MyAudioClient : public IAudioClient
{
public:
	HRESULT __stdcall MyGetCurrentPadding(UINT32* pNumPaddingFrames)
	{
		if (autoSelfRegister())
			std::cout << "Registered AudioClient 0x" << std::hex << this << std::endl;
		return (this->*realGetCurrentPadding)(pNumPaddingFrames);
	}
private:
	bool autoSelfRegister()
	{
		std::lock_guard lock(gMtxAudioClients);
		if (gAudioClients.find(this) != gAudioClients.end())
			return false;

		auto pwfx = getInternalWFX();
		std::cout << "  Format:" << std::endl
			<< "    Format:          0x" << std::hex << pwfx->wFormatTag << std::dec << std::endl
			<< "    Channels:        " << pwfx->nChannels << std::endl
			<< "    Samplerate:      " << pwfx->nSamplesPerSec << " Hz" << std::endl
			<< "    BytesPerSec:     " << pwfx->nAvgBytesPerSec << std::endl
			<< "    Samplesize:      " << pwfx->nBlockAlign << std::endl
			<< "    BytesPerChannel: " << (pwfx->wBitsPerSample / 8) << std::endl;
		if (pwfx->wFormatTag != WAVE_FORMAT_PCM && pwfx->cbSize > 0)
		{
			auto pwfxx = (WAVEFORMATEXTENSIBLE*)pwfx;
			std::cout
				<< "    ValidBitsPerSample: " << pwfxx->Samples.wValidBitsPerSample << std::endl
				<< "    SamplesPerBlock:    " << pwfxx->Samples.wSamplesPerBlock << std::endl
				<< "    ChannelMask:        " << pwfxx->dwChannelMask << std::endl
				<< "    SubFormat:          ";
			for (int i = 0; i < sizeof(GUID); ++i)
				std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)*((char*)&pwfxx->SubFormat + i) << " ";
			std::cout << std::dec << std::endl;
		}

		gAudioClients.insert({ this, pwfx });

		outFileStream = std::ofstream("C:\\Users\\tecst\\Desktop\\output.raw", std::ios::binary | std::ios::out | std::ios::trunc);
		
		return true;
	}
	WAVEFORMATEX* getInternalWFX()
	{
		WAVEFORMATEXTENSIBLE;
		// Pointer to internal waveformatex struct is stored at this+PWFX_OFFSET
		char* pData = (char*)this;
		return *(WAVEFORMATEX**)(pData + PWFX_OFFSET);
	}
public:
	inline static MyAudioClient_GetCurrentPadding_t realGetCurrentPadding = 0; // vtable[6]
	inline static const MyAudioClient_GetCurrentPadding_t pfMyGetCurrentPadding = &MyAudioClient::MyGetCurrentPadding;
private:
	#if defined PAYLOAD_ARCH_x64
	inline static const uint64_t PWFX_OFFSET = 904;
	#elif defined PAYLOAD_ARCH_x86
	inline static const uint64_t PWFX_OFFSET = 660;
	#elif defined PAYLOAD_ARCH_UNKNOWN
	inline static const uint64_t PWFX_OFFSET = 0;
	#endif
};

class MyAudioRenderClient : public IAudioRenderClient
{
public:
	HRESULT __stdcall MyGetBuffer(UINT32 NumFramesRequested, BYTE** ppData)
	{
		if (autoSelfRegister())
			std::cout << "Registered RenderClient 0x" << std::hex << this << std::dec << std::endl;
		HRESULT hr = (this->*realGetBuffer)(NumFramesRequested, ppData);
		BufferData = *ppData;
		return hr;
	}
	HRESULT __stdcall MyReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags)
	{
		if (gRenderClients.find(this) == gRenderClients.begin() && !gAudioClients.empty())
		{
			auto pwfx = gAudioClients.begin()->second;
			uint64_t frameSize = pwfx->nChannels * (pwfx->wBitsPerSample / 8);
			outFileStream.write((char*)BufferData, NumFramesWritten * frameSize);
			outFileStream.flush();
		}
		return (this->*realReleaseBuffer)(NumFramesWritten, dwFlags);
	}
private:
	bool autoSelfRegister()
	{
		std::lock_guard lock(gMtxRenderClients);
		if (gRenderClients.find(this) != gRenderClients.end())
			return false;
		gRenderClients.insert(this);
		return true;
	}
public:
	inline static BYTE* BufferData = 0;
	inline static MyAudioRenderClient_GetBuffer_t realGetBuffer = 0; // vtable[3]
	inline static const MyAudioRenderClient_GetBuffer_t pfMyGetBuffer = &MyAudioRenderClient::MyGetBuffer;
	inline static MyAudioRenderClient_ReleaseBuffer_t realReleaseBuffer = 0; // vtable[4]
	inline static const MyAudioRenderClient_ReleaseBuffer_t pfMyReleaseBuffer = &MyAudioRenderClient::MyReleaseBuffer;
};

IAudioClient* createDummyAudioClient()
{
	HRESULT hr = ERROR_SUCCESS;
	hr = CoInitialize(NULL);

	REFERENCE_TIME hnsRequestedDuration = 10000000;
	IMMDeviceEnumerator* pEnumerator = nullptr;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	WAVEFORMATEX* pwfx = nullptr;

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator
	);
	if (!SUCCEEDED(hr))
		std::cout << "CoCreateInstance returned with code " << std::hex << hr << std::endl;

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice
	);
	if (!SUCCEEDED(hr))
		std::cout << "pEnumerator->GetDefaultAudioEndpoint returned with code " << std::hex << hr << std::endl;

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient
	);
	if (!SUCCEEDED(hr))
		std::cout << "pDevice->Activate returned with code " << std::hex << hr << std::endl;

	hr = pAudioClient->GetMixFormat(&pwfx);
	if (!SUCCEEDED(hr))
		std::cout << "pAudioClient->GetMixFormat returned with code " << std::hex << hr << std::endl;

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		pwfx,
		nullptr
	);
	if (!SUCCEEDED(hr))
		std::cout << "pAudioClient->Initialize returned with code " << std::hex << hr << std::endl;

	CoTaskMemFree(pwfx);
	pEnumerator->Release();
	pDevice->Release();

	return pAudioClient;
}

void destroyDummyAudioClient(IAudioClient* pAudioClient)
{
	pAudioClient->Release();
}

IAudioRenderClient* createDummyAudioRenderClient(IAudioClient* pAudioClient)
{
	HRESULT hr = ERROR_SUCCESS;
	
	IAudioRenderClient* pAudioRenderClient = nullptr;

	hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pAudioRenderClient);
	if (!SUCCEEDED(hr))
		std::cout << "pAudioClient->GetService returned with code " << std::hex << hr << std::endl;

	return pAudioRenderClient;
}

void destroyDummyAudioRenderClient(IAudioRenderClient* pClient)
{
	pClient->Release();
}

void makeAudioRenderClientDetours()
{
	IAudioClient* pAudioClient = createDummyAudioClient();
	IAudioRenderClient* pAudioRenderClient = createDummyAudioRenderClient(pAudioClient);
	MyAudioClient::realGetCurrentPadding = GetFuncFromObjVTable<MyAudioClient_GetCurrentPadding_t>(pAudioClient, 6);
	MyAudioRenderClient::realGetBuffer = GetFuncFromObjVTable<MyAudioRenderClient_GetBuffer_t>(pAudioRenderClient, 3);
	MyAudioRenderClient::realReleaseBuffer = GetFuncFromObjVTable<MyAudioRenderClient_ReleaseBuffer_t>(pAudioRenderClient, 4);
	destroyDummyAudioRenderClient(pAudioRenderClient);
	destroyDummyAudioClient(pAudioClient);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)MyAudioClient::realGetCurrentPadding, *(PBYTE*)&MyAudioClient::pfMyGetCurrentPadding);
	DetourAttach(&(PVOID&)MyAudioRenderClient::realGetBuffer, *(PBYTE*)&MyAudioRenderClient::pfMyGetBuffer);
	DetourAttach(&(PVOID&)MyAudioRenderClient::realReleaseBuffer, *(PBYTE*)&MyAudioRenderClient::pfMyReleaseBuffer);
	DetourTransactionCommit();
}

void removeAudioRenderClientDetours()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)MyAudioClient::realGetCurrentPadding, *(PBYTE*)&MyAudioClient::pfMyGetCurrentPadding);
	DetourDetach(&(PVOID&)MyAudioRenderClient::realGetBuffer, *(PBYTE*)&MyAudioRenderClient::pfMyGetBuffer);
	DetourDetach(&(PVOID&)MyAudioRenderClient::realReleaseBuffer, *(PBYTE*)&MyAudioRenderClient::pfMyReleaseBuffer);
	DetourTransactionCommit();
}

void SearchWaveFormatExPtrInAudioClient(IAudioClient* pAudioClient)
{
	WAVEFORMATEX* pwfx; // fe ff 08 00 80 bb 00 00 00 70 17 00 20 00 20 00 16 00
	pAudioClient->GetMixFormat(&pwfx);
	
	std::set<uintptr_t> toCheck;
	while (true)
	{
		uintptr_t ptr;
		std::cout << " > ";
		std::cin >> std::hex;
		std::cin >> ptr;
		if (ptr == 0)
			break;
		toCheck.insert(ptr);
	}

	char* pRawAudioClient = (char*)pAudioClient;
	{
		uint64_t offset = 0;
		while (offset < 1024)
		{
			uintptr_t* curr = (uintptr_t*)(pRawAudioClient + offset);
			if (toCheck.find(*curr) != toCheck.end())
				std::cout << "Found match at offset " << std::dec << offset << std::endl;
			++offset;
		}
	}
	CoTaskMemFree(pwfx);
}