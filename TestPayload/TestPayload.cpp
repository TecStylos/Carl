#include <CarlPayload.h>

#include <iostream>
#include <iomanip>
#include <string>

#include "MyAudioClient.h"
#include "CarlEntryPoint.h"

static EHSN::net::ManagedSocketRef gQueue = nullptr;
static EHSN::CircularBuffer gCaptureCircularBuffer(1024 * 1024);
static bool gIsReadingRenderFrames = false;
static bool gIsWritingCaptureFrames = false;
static WAVEFORMATEX* gpWfx = nullptr;
static std::mutex gMtxAudioClients;
static std::mutex gMtxRenderClients;
static std::mutex gMtxCaptureClients;
static std::map<MyAudioClient*, WAVEFORMATEX*> gAudioClients;
static std::set<MyAudioRenderClient*> gRenderClients;
static std::set<MyAudioCaptureClient*> gCaptureClients;

namespace Carl
{
	bool CreateConsole = true;

	int MainFunc(void* param)
	{
		std::string port = (char*)param;

		gQueue = std::make_shared<EHSN::net::ManagedSocket>(std::make_shared<EHSN::net::SecSocket>(EHSN::crypto::defaultRDG, 0));

		uint8_t connectCount = 0;
		while (!gQueue->isConnected() && connectCount++ < 8)
			gQueue->connect("localhost", port, true);

		if (!gQueue->isConnected())
			return -1;

		gQueue->setRecvCallback(
			Carl::PT_WAVEFORMATEX_REQUEST,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				if (nBytesReceived < pack.header.packetSize)
					return;

				if (!gpWfx)
				{
					gQueue->push(Carl::PT_WAVEFORMATEX_REPLY, EHSN::net::FLAG_PH_NONE, nullptr);
					return;
				}
				auto buffer = std::make_shared<EHSN::net::PacketBuffer>(sizeof(WAVEFORMATEX));
				buffer->write(*gpWfx);
				gQueue->push(Carl::PT_WAVEFORMATEX_REPLY, EHSN::net::FLAG_PH_NONE, buffer);
			},
			nullptr
				);

		gQueue->setRecvCallback(
			Carl::PT_START_RENDER_FRAME,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				gIsReadingRenderFrames = true;
			},
			nullptr
				);
		gQueue->setRecvCallback(
			Carl::PT_STOP_RENDER_FRAME,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				gIsReadingRenderFrames = false;
			},
			nullptr
				);
		gQueue->setRecvCallback(
			Carl::PT_START_CAPTURE_FRAME,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				gIsWritingCaptureFrames = true;
			},
			nullptr
				);
		gQueue->setRecvCallback(
			Carl::PT_STOP_CAPTURE_FRAME,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				gIsWritingCaptureFrames = false;
			},
			nullptr
				);
		gQueue->setRecvCallback(
			Carl::PT_CAPTURE_FRAME,
			[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
				if (nBytesReceived < pack.header.packetSize)
					return;
				gCaptureCircularBuffer.write(pack.buffer->data(), nBytesReceived);
			},
			nullptr
				);
		

		std::cout << "Connected to host!" << std::endl;

		makeAudioRenderClientDetours();

		while (gQueue->isConnected())
		{
			auto pack = gQueue->pull(Carl::PT_ECHO_REQUEST);
			if (!pack.buffer)
				break;
			std::string line = (char*)pack.buffer->data();
			std::cout << line << std::endl;
		}

		gQueue.reset();

		return 0;
	}
}

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


	gpWfx = pwfx;

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

bool autoRegisterAudioCaptureClient(MyAudioCaptureClient* pClient)
{
	std::lock_guard lock(gMtxCaptureClients);
	if (gCaptureClients.find(pClient) != gCaptureClients.end())
		return false;
	gCaptureClients.insert(pClient);
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
	if (gRenderClients.find(pClient) == gRenderClients.begin() &&
		!gAudioClients.empty() &&
		gQueue.get() != nullptr &&
		gQueue->isConnected() &&
		gIsReadingRenderFrames
		)
	{
		auto pwfx = gAudioClients.begin()->second;
		uint64_t frameSize = pwfx->nChannels * (pwfx->wBitsPerSample / 8);

		auto buffer = std::make_shared<EHSN::net::PacketBuffer>(NumFramesWritten * frameSize);
		buffer->write(BufferData, NumFramesWritten * frameSize, 0);
		gQueue->push(Carl::PT_RENDER_FRAME, EHSN::net::FLAG_PH_NONE, buffer);
		memset(BufferData, 0, NumFramesWritten * frameSize);
	}
	return sDetourReleaseBuffer->callReal<HRESULT>(pClient, NumFramesWritten, dwFlags);
};

MyAudioCaptureClient::CbGetBuffer_t MyAudioCaptureClient::sCbGetBuffer = [](MyAudioCaptureClient* pClient, BYTE** ppData, UINT32* pNumFramesToRead, DWORD* pdwFlags, UINT64* pu64DevicePosition, UINT64* pu64QPCPosition) {
	if (autoRegisterAudioCaptureClient(pClient))
		std::cout << "Registered CaptureClient 0x" << std::hex << pClient << std::dec << std::endl;
	HRESULT hr = sDetourGetBuffer->callReal<HRESULT>(pClient, ppData, pNumFramesToRead, pdwFlags, pu64DevicePosition, pu64QPCPosition);
	if (gIsWritingCaptureFrames && gpWfx && *pNumFramesToRead)
	{
		uint64_t nBytesToRead = *pNumFramesToRead * (gpWfx->wBitsPerSample / 8);
		sFakeData = new char[nBytesToRead];
		gCaptureCircularBuffer.read(sFakeData, nBytesToRead);
		*ppData = (BYTE*)sFakeData;
	}
	return hr;
};

MyAudioCaptureClient::CbReleaseBuffer_t MyAudioCaptureClient::sCbReleaseBuffer = [](MyAudioCaptureClient* pClient, UINT32 NumFramesRead) {
	if (gIsWritingCaptureFrames)
		delete[] sFakeData;
	return sDetourReleaseBuffer->callReal<HRESULT>(pClient, NumFramesRead);
};