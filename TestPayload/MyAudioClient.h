#pragma once

#include <iostream>
#include <iomanip>
#include <set>
#include <map>
#include <mutex>

#include <Audioclient.h>
#include <mmdeviceapi.h>

#include "CarlDetour.h"
#include "GetFuncFromObjVTable.h"

#if !defined(PAYLOAD_ARCH_x86) && !defined(PAYLOAD_ARCH_x64)
#define PAYLOAD_ARCH_UNKNOWN
#endif

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
const IID IID_IAudioStreamVolume = __uuidof(IAudioStreamVolume);

#define CARL_METHOD_SETUP(className, retType, methodName, ...) \
typedef retType(__stdcall className::* methodName##_t)(__VA_ARGS__); \
typedef retType(* Cb##methodName##_t)(className* pIntance, __VA_ARGS__); \
inline static CarlDetourRef<methodName##_t> sDetour##methodName = nullptr; \
static retType(*sCb##methodName)(className* pIntance, __VA_ARGS__)

class MyAudioClient : public IAudioClient
{
public:
	CARL_METHOD_SETUP(MyAudioClient, HRESULT, GetCurrentPadding, UINT32* pNumPaddingFrames);
	HRESULT __stdcall MyGetCurrentPadding(UINT32* pNumPaddingFrames)
	{
		return sCbGetCurrentPadding(this, pNumPaddingFrames);
	}
public:
	WAVEFORMATEX* getInternalWFX()
	{
		// Pointer to internal waveformatex struct is stored at this+PWFX_OFFSET
		char* pData = (char*)this;
		return *(WAVEFORMATEX**)(pData + PWFX_OFFSET);
	}
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
	CARL_METHOD_SETUP(MyAudioRenderClient, HRESULT, GetBuffer, UINT32 NumFramesRequested, BYTE** ppData);
	HRESULT __stdcall MyGetBuffer(UINT32 NumFramesRequested, BYTE** ppData)
	{
		return sCbGetBuffer(this, NumFramesRequested, ppData);
	}
	CARL_METHOD_SETUP(MyAudioRenderClient, HRESULT, ReleaseBuffer, UINT32 NumFramesWritten, DWORD dwFlags);
	HRESULT __stdcall MyReleaseBuffer(UINT32 NumFramesWritten, DWORD dwFlags)
	{
		return sCbReleaseBuffer(this, NumFramesWritten, dwFlags);
	}
public:
	inline static BYTE* BufferData = 0;
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
		return nullptr;

	hr = pEnumerator->GetDefaultAudioEndpoint(
		eRender, eConsole, &pDevice
	);
	if (!SUCCEEDED(hr))
		return nullptr;

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient
	);
	if (!SUCCEEDED(hr))
		return nullptr;

	hr = pAudioClient->GetMixFormat(&pwfx);
	if (!SUCCEEDED(hr))
		return nullptr;

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		0,
		hnsRequestedDuration,
		0,
		pwfx,
		nullptr
	);
	if (!SUCCEEDED(hr))
		return nullptr;

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
		return nullptr;

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

	MyAudioClient::sDetourGetCurrentPadding = std::make_shared<CarlDetour<MyAudioClient::GetCurrentPadding_t>>(
		GetFuncFromObjVTable<MyAudioClient::GetCurrentPadding_t>(pAudioClient, 6),
		&MyAudioClient::MyGetCurrentPadding
		);
	MyAudioRenderClient::sDetourGetBuffer = std::make_shared<CarlDetour<MyAudioRenderClient::GetBuffer_t>>(
		GetFuncFromObjVTable<MyAudioRenderClient::GetBuffer_t>(pAudioRenderClient, 3),
		&MyAudioRenderClient::MyGetBuffer
		);
	MyAudioRenderClient::sDetourReleaseBuffer = std::make_shared<CarlDetour<MyAudioRenderClient::ReleaseBuffer_t>>(
		GetFuncFromObjVTable<MyAudioRenderClient::ReleaseBuffer_t>(pAudioRenderClient, 4),
		&MyAudioRenderClient::MyReleaseBuffer
		);

	destroyDummyAudioRenderClient(pAudioRenderClient);
	destroyDummyAudioClient(pAudioClient);
}

uint64_t SearchWaveFormatExPtrInAudioClient(IAudioClient* pAudioClient)
{
	WAVEFORMATEX* pwfx; // Example: fe ff 08 00 80 bb 00 00 00 70 17 00 20 00 20 00 16 00
	pAudioClient->GetMixFormat(&pwfx);
	
	std::set<uintptr_t> toCheck;
	// TODO: Insert addresses of *pwfx occurences into toCheck

	char* pRawAudioClient = (char*)pAudioClient;
	{
		uint64_t offset = 0;
		while (offset < 1024)
		{
			uintptr_t* curr = (uintptr_t*)(pRawAudioClient + offset);
			if (toCheck.find(*curr) != toCheck.end())
				return offset;
			++offset;
		}
	}
	CoTaskMemFree(pwfx);
	return -1;
}