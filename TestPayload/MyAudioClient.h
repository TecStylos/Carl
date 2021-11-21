#pragma once

#include <set>
#include <iostream>

#include <Audioclient.h>
#include <mmdeviceapi.h>

#include "CarlDetour.h"
#include "GetFuncFromObjVTable.h"

#if !defined(PAYLOAD_ARCH_x86) && !defined(PAYLOAD_ARCH_x64)
#define PAYLOAD_ARCH_UNKNOWN
#endif

const CLSID CLSID_MMDeviceEnumerator  = __uuidof(MMDeviceEnumerator);
const IID     IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID     IID_IAudioClient        = __uuidof(IAudioClient);
const IID     IID_IAudioRenderClient  = __uuidof(IAudioRenderClient);
const IID     IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);
const IID     IID_IAudioStreamVolume  = __uuidof(IAudioStreamVolume);

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

class MyAudioCaptureClient : public IAudioCaptureClient
{
public:
	CARL_METHOD_SETUP(MyAudioCaptureClient, HRESULT, GetBuffer, BYTE** ppData, UINT32* pNumFramesToRead, DWORD* pdwFlags, UINT64* pu64DevicePosition, UINT64* pu64QPCPosition);
	HRESULT __stdcall MyGetBuffer(BYTE** ppData, UINT32* pNumFramesToRead, DWORD* pdwFlags, UINT64* pu64DevicePosition, UINT64* pu64QPCPosition)
	{
		return sCbGetBuffer(this, ppData, pNumFramesToRead, pdwFlags, pu64DevicePosition, pu64QPCPosition);
	}
	CARL_METHOD_SETUP(MyAudioCaptureClient, HRESULT, ReleaseBuffer, UINT32 NumFramesRead);
	HRESULT __stdcall MyReleaseBuffer(UINT32 NumFramesRead)
	{
		return sCbReleaseBuffer(this, NumFramesRead);
	}
public:
	inline static void* sFakeData = 0;
};

IAudioClient* createDummyAudioClient(EDataFlow dataFlow)
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
		dataFlow, eConsole, &pDevice
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
	if (!pAudioClient)
		return nullptr;
	HRESULT hr = ERROR_SUCCESS;
	
	IAudioRenderClient* pAudioRenderClient = nullptr;

	hr = pAudioClient->GetService(IID_IAudioRenderClient, (void**)&pAudioRenderClient);
	if (!SUCCEEDED(hr))
		return nullptr;

	return pAudioRenderClient;
}

IAudioCaptureClient* createDummyAudioCaptureClient(IAudioClient* pAudioClient)
{
	if (!pAudioClient)
		return nullptr;
	HRESULT hr = ERROR_SUCCESS;

	IAudioCaptureClient* pAudioCaptureClient = nullptr;

	hr = pAudioClient->GetService(IID_IAudioCaptureClient, (void**)&pAudioCaptureClient);
	if (!SUCCEEDED(hr))
		return nullptr;

	return pAudioCaptureClient;
}

void destroyDummyAudioRenderClient(IAudioRenderClient* pClient)
{
	if (!pClient)
		return;
	pClient->Release();
}

void destroyDummyAudioCaptureClient(IAudioCaptureClient* pClient)
{
	if (!pClient)
		return;
	pClient->Release();
}

bool makeAudioRenderClientDetours()
{
	IAudioClient* pAudioClientForRender = createDummyAudioClient(eRender);
	if (!pAudioClientForRender)
		goto ErrExit;

	IAudioRenderClient* pAudioRenderClient = createDummyAudioRenderClient(pAudioClientForRender);
	if (!pAudioRenderClient)
		goto ErrExit;

	IAudioClient* pAudioClientForCapture = createDummyAudioClient(eCapture);
	if (!pAudioClientForCapture)
		goto ErrExit;

	IAudioCaptureClient* pAudioCaptureClient = createDummyAudioCaptureClient(pAudioClientForCapture);
	if (!pAudioCaptureClient)
		goto ErrExit;

	CREATE_CARL_DETOUR_REF(MyAudioClient, GetCurrentPadding, GetFuncFromObjVTable<MyAudioClient::GetCurrentPadding_t>(pAudioClientForRender, 6));
	CREATE_CARL_DETOUR_REF(MyAudioRenderClient, GetBuffer, GetFuncFromObjVTable<MyAudioRenderClient::GetBuffer_t>(pAudioRenderClient, 3));
	CREATE_CARL_DETOUR_REF(MyAudioRenderClient, ReleaseBuffer, GetFuncFromObjVTable<MyAudioRenderClient::ReleaseBuffer_t>(pAudioRenderClient, 4));
	CREATE_CARL_DETOUR_REF(MyAudioCaptureClient, GetBuffer, GetFuncFromObjVTable<MyAudioCaptureClient::GetBuffer_t>(pAudioCaptureClient, 3));
	CREATE_CARL_DETOUR_REF(MyAudioCaptureClient, ReleaseBuffer, GetFuncFromObjVTable<MyAudioCaptureClient::ReleaseBuffer_t>(pAudioCaptureClient, 4));

	if (!MyAudioClient::sDetourGetCurrentPadding->isDetoured() ||
		!MyAudioRenderClient::sDetourGetBuffer->isDetoured() ||
		!MyAudioRenderClient::sDetourReleaseBuffer->isDetoured() ||
		!MyAudioCaptureClient::sDetourGetBuffer->isDetoured() ||
		!MyAudioCaptureClient::sDetourReleaseBuffer->isDetoured()
		)
		goto ErrExit;

	destroyDummyAudioRenderClient(pAudioRenderClient);
	destroyDummyAudioCaptureClient(pAudioCaptureClient);
	destroyDummyAudioClient(pAudioClientForRender);
	destroyDummyAudioClient(pAudioClientForCapture);
	return true;
ErrExit:
	destroyDummyAudioRenderClient(pAudioRenderClient);
	destroyDummyAudioCaptureClient(pAudioCaptureClient);
	destroyDummyAudioClient(pAudioClientForRender);
	destroyDummyAudioClient(pAudioClientForCapture);

	MyAudioClient::sDetourGetCurrentPadding.reset();
	MyAudioRenderClient::sDetourGetBuffer.reset();
	MyAudioRenderClient::sDetourReleaseBuffer.reset();
	MyAudioCaptureClient::sDetourGetBuffer.reset();
	MyAudioCaptureClient::sDetourReleaseBuffer.reset();
	return false;
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