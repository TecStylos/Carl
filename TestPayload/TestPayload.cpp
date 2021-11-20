#include <CarlPayload.h>

#include <iostream>
#include <string>
#include <fstream>

#include "MyAudioClient.h"
#include "CarlEntryPoint.h"

namespace Carl
{
	bool CreateConsole = true;

	int MainFunc(void* param)
	{
		std::string port = (char*)param;

		makeAudioRenderClientDetours();

		EHSN::net::ManagedSocket queue(std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0));

		uint8_t connectCount = 0;
		while (!queue.isConnected() && connectCount++ < 8)
			queue.connect("localhost", port, true);

		if (!queue.isConnected())
			return -1;

		std::cout << "Connected to host!" << std::endl;

		while (queue.isConnected())
		{
			auto pack = queue.pull(Carl::PT_ECHO_REQUEST);
			if (!pack.buffer)
				break;
			std::string line = (char*)pack.buffer->data();
			std::cout << line << std::endl;
		}

		return 0;
	}
}

std::mutex gMtxAudioClients;
std::mutex gMtxRenderClients;
static std::map<IAudioClient*, WAVEFORMATEX*> gAudioClients;
static std::set<IAudioRenderClient*> gRenderClients;

std::ofstream outFileStream;

bool autoRegisterAudioClient(MyAudioClient* pClient)
{
	std::lock_guard lock(gMtxAudioClients);
	if (gAudioClients.find(pClient) != gAudioClients.end())
		return false;

	auto pwfx = pClient->getInternalWFX();
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

	gAudioClients.insert({ pClient, pwfx });

	outFileStream = std::ofstream("C:\\Users\\tecst\\Desktop\\output.raw", std::ios::binary | std::ios::out | std::ios::trunc);

	return true;
}

bool autoRegisterAudioRenderClient(MyAudioRenderClient* pClient)
{
	std::lock_guard lock(gMtxRenderClients);
	if (gRenderClients.find(pClient) != gRenderClients.end())
		return false;
	gRenderClients.insert(pClient);
	return true;
}

MyAudioClient::CbGetCurrentPadding_t MyAudioClient::sCbGetCurrentPadding = [](MyAudioClient* pClient, UINT32* pNumPaddingFrames) {
	if (autoRegisterAudioClient(pClient))
		std::cout << "Registered AudioClient 0x" << std::hex << pClient << std::endl;
	return sDetourGetCurrentPadding->callReal<HRESULT>(pClient, pNumPaddingFrames);
};

MyAudioRenderClient::CbGetBuffer_t MyAudioRenderClient::sCbGetBuffer = [](MyAudioRenderClient* pClient, UINT32 NumFramesRequested, BYTE** ppData) {

	if (autoRegisterAudioRenderClient(pClient))
		std::cout << "Registered RenderClient 0x" << std::hex << pClient << std::dec << std::endl;
	HRESULT hr = sDetourGetBuffer->callReal<HRESULT>(pClient, NumFramesRequested, ppData);
	BufferData = *ppData;
	return hr;
};

MyAudioRenderClient::CbReleaseBuffer_t MyAudioRenderClient::sCbReleaseBuffer = [](MyAudioRenderClient* pClient, UINT32 NumFramesWritten, DWORD dwFlags) {
	if (gRenderClients.find(pClient) == gRenderClients.begin() && !gAudioClients.empty())
	{
		auto pwfx = gAudioClients.begin()->second;
		uint64_t frameSize = pwfx->nChannels * (pwfx->wBitsPerSample / 8);
		outFileStream.write((char*)BufferData, NumFramesWritten * frameSize);
		outFileStream.flush();
	}
	return sDetourReleaseBuffer->callReal<HRESULT>(pClient, NumFramesWritten, dwFlags);
};