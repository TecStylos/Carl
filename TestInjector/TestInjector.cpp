#include <iostream>

#include <set>
#include <Carl.h>
#include <CarlPayload.h>

#include <Audioclient.h>

#include <fstream>

std::ofstream outFileStream;

enum class WaveBaseType
{
	None = 0,
	Int8,
	Int16,
	Int32,
	Int64,
	Float,
	Double
} gWbt = WaveBaseType::None;

uint64_t WaveBaseTypeSize(WaveBaseType wbt)
{
	switch (wbt)
	{
	case WaveBaseType::None: return 0;
	case WaveBaseType::Int8: return 1;
	case WaveBaseType::Int16: return 2;
	case WaveBaseType::Int32: return 4;
	case WaveBaseType::Int64: return 8;
	case WaveBaseType::Float: return 4;
	case WaveBaseType::Double: return 8;
	}
	return 0;
}

bool receivedWfx = false;
WAVEFORMATEX wfx;

template <typename T>
bool isFloatingPointBuffer(const void* buffer, uint64_t nFrames, uint64_t nChannels)
{
	uint64_t frameSize = nChannels * sizeof(T);
	uint64_t nInRange = 0;

	for (uint64_t i = 0; i < nFrames * nChannels; ++i)
	{
		T toCheck = *((T*)buffer + i);
		if (-1.0f <= toCheck && toCheck <= 1.0f)
			++nInRange;
	}

	return nInRange > (nFrames * nChannels * 9 / 10);
}
template <typename T>
constexpr uint64_t WaveChannelSize()
{
	return sizeof(T);
}
template <typename T>
constexpr uint64_t WaveFrameSize(uint64_t nChannels)
{
	return WaveChannelSize<T>() * nChannels;
}

template<typename To, typename From, uint64_t ToMax, uint64_t FromMax>
void convertBuffer(void* destBuffer, const void* srcBuffer, uint64_t nChannels, uint64_t nFrames)
{
	static constexpr uint64_t destChannelSize = WaveChannelSize<To>();
	static constexpr uint64_t srcChannelSize = WaveChannelSize<From>();
	const uint64_t destFrameSize = WaveFrameSize<To>(nChannels);
	const uint64_t srcFrameSize = WaveFrameSize<From>(nChannels);

	static constexpr float ConvRatio = (float)ToMax / (float)FromMax;

	auto pDest = (To*)destBuffer;
	auto pSrc = (From*)srcBuffer;
	for (uint64_t i = 0; i < nFrames * nChannels; ++i)
	{
		float temp = *(pSrc + i);
		temp *= ConvRatio;
		*(pDest + i) = temp;
	}
}

WaveBaseType detectWaveBaseType(WAVEFORMATEX* pwfx, const void* buffer, uint64_t bufferSize)
{
	uint64_t channelSize = pwfx->wBitsPerSample / 8;

	switch (channelSize)
	{
	case 1:
		return WaveBaseType::Int8;
	case 2:
		return WaveBaseType::Int16;
	case 4:
		if (isFloatingPointBuffer<float>(buffer, bufferSize / (channelSize * pwfx->nChannels), pwfx->nChannels))
			return WaveBaseType::Float;
		return WaveBaseType::Int32;
	case 8:
		if (isFloatingPointBuffer<double>(buffer, bufferSize / (channelSize * pwfx->nChannels), pwfx->nChannels))
			return WaveBaseType::Double;
		return WaveBaseType::Int64;
	}

	return WaveBaseType::None;
}

void RenderFrameCallback(EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam)
{
	if (nBytesReceived < pack.header.packetSize)
		return;

	if (gWbt == WaveBaseType::None)
	{
		if (!receivedWfx)
			return;
		gWbt = detectWaveBaseType(&wfx, pack.buffer->data(), pack.buffer->size());
		std::cout << "WaveBaseType = " << (int)gWbt << std::endl;
	}
	if (gWbt != WaveBaseType::Float)
	{
		std::cout << "Wrong WaveBaseType!" << std::endl;
		return;
	}

	uint64_t nFrames = nBytesReceived / (wfx.nChannels * wfx.wBitsPerSample / 8);
	auto outBuffer = new char[nFrames * WaveFrameSize<int16_t>(wfx.nChannels)];

	convertBuffer<int16_t, float, 32767, 1>(outBuffer, pack.buffer->data(), wfx.nChannels, nFrames);
	outFileStream.write(outBuffer, nFrames * wfx.nChannels * WaveChannelSize<int16_t>());
	outFileStream.flush();
	delete[] outBuffer;
}

int main()
{
	std::string agentDir = "..\\..\\..\\bintools\\Debug\\";

	std::cout << "Target PID: ";
	std::string tPIDStr;
	std::getline(std::cin, tPIDStr);
	Carl::ProcessID targetPID;
	targetPID = std::stoul(tPIDStr);

	std::string payloadPath = "..\\..\\..\\bintools\\Debug\\TestPayload_[.].dll";

	Carl::PayloadConnectorRef pc;

	try
	{
		pc = Carl::injectPayload(agentDir, targetPID, payloadPath);
	}
	catch (const Carl::CarlError& err)
	{
		std::cout << "ERROR: " << err.what() << std::endl;
		return -1;
	}

	std::cout << "PayloadConnector port: " << pc->getPort() << std::endl;

	auto queue = pc->getQueue();

	outFileStream = std::ofstream("C:\\Users\\tecst\\Desktop\\output.raw", std::ios::binary | std::ios::out | std::ios::trunc);
	queue->setRecvCallback(Carl::PT_RENDER_FRAME, RenderFrameCallback, nullptr);
	queue->setRecvCallback(
		Carl::PT_WAVEFORMATEX,
		[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
			if (nBytesReceived < pack.header.packetSize)
				return;
			pack.buffer->read(wfx);
			receivedWfx = true;
		},
		nullptr
			);

	while (queue->isConnected())
	{
		std::string line;
		std::cout << " >>> ";
		std::getline(std::cin, line);
		auto pBuffer = std::make_shared<EHSN::net::PacketBuffer>(line.size() + 1);
		pBuffer->write(line.c_str(), line.size() + 1, 0);
		queue->push(Carl::PT_ECHO_REQUEST, EHSN::net::FLAG_PH_NONE, pBuffer);
	}

	return 0;
}