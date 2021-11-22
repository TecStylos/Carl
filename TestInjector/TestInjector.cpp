#include <iostream>

#include <set>
#include <Carl.h>
#include <CarlPayload.h>

#include <Audioclient.h>

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
struct Connections
{
	EHSN::net::ManagedSocket* queueSpotify;
	EHSN::net::ManagedSocket* queueDiscord;
};

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
void convertBuffer(void* destBuffer, const void* srcBuffer, uint64_t nChannels, uint64_t nFrames, bool makeMono)
{
	const uint64_t nDestChannels = makeMono ? 1 : nChannels;

	static constexpr float ConvRatio = (float)ToMax / (float)FromMax;

	auto pDest = (To*)destBuffer;
	auto pSrc = (From*)srcBuffer;

	void (*convertFrame)(To*, From*, uint64_t) = [](To* pDest, From* pSrc, uint64_t nChannels) {
		for (uint64_t channel = 0; channel < nChannels; ++channel)
		{
			float temp = *(pSrc + channel);
			temp *= ConvRatio;
			*(pDest + channel) = temp;
		}
	};

	if (makeMono)
	{
		convertFrame = [](To* pDest, From* pSrc, uint64_t nChannels) {
			float temp = *(pSrc);
			for (uint64_t channel = 0; channel < nChannels; ++channel)
				temp += *(pSrc + channel);
			temp *= ConvRatio;
			temp /= nChannels;
			*(pDest) = temp;
		};
	}

	for (uint64_t i = 0; i < nFrames; ++i)
	{
		convertFrame(pDest, pSrc, nChannels);
		pDest += nDestChannels;
		pSrc += nChannels;
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
	Connections* pCons = (Connections*)pParam;
	if (nBytesReceived < pack.header.packetSize)
		return;

	if (gWbt == WaveBaseType::None)
	{
		if (!receivedWfx)
		{
			pCons->queueSpotify->push(Carl::PT_WAVEFORMATEX_REQUEST, EHSN::net::FLAG_PH_NONE, nullptr);
			return;
		}
		gWbt = detectWaveBaseType(&wfx, pack.buffer->data(), pack.buffer->size());
		std::cout << "WaveBaseType = " << (int)gWbt << std::endl;
	}
	if (gWbt != WaveBaseType::Float)
	{
		std::cout << "Wrong WaveBaseType!" << std::endl;
		return;
	}

	uint64_t nFrames = nBytesReceived / (wfx.nChannels * wfx.wBitsPerSample / 8);
	auto buffer = std::make_shared<EHSN::net::PacketBuffer>(nFrames * WaveFrameSize<int16_t>(1));

	convertBuffer<int16_t, float, 32767, 1>(buffer->data(), pack.buffer->data(), wfx.nChannels, nFrames, true);

	pCons->queueDiscord->push(Carl::PT_CAPTURE_FRAME, EHSN::net::FLAG_PH_NONE, buffer);
}

int main()
{
	std::string agentDir = "..\\..\\..\\bintools\\Debug\\";

	std::string strPIDSpotify, strPIDDiscord;
	Carl::ProcessID PIDSpotify, PIDDiscord;
	std::cout << "Spotify PID: ";
	std::getline(std::cin, strPIDSpotify);
	std::cout << "Discord PID: ";
	std::getline(std::cin, strPIDDiscord);
	PIDSpotify = std::stoul(strPIDSpotify);
	PIDDiscord = std::stoul(strPIDDiscord);

	std::string payloadPath = "..\\..\\..\\bintools\\Debug\\TestPayload_[.].dll";

	Carl::PayloadConnectorRef pcSpotify, pcDiscord;

	try
	{
		pcSpotify = Carl::injectPayload(agentDir, PIDSpotify, payloadPath);
	}
	catch (const Carl::CarlError& err)
	{
		std::cout << "ERROR Injecting into Spotify: " << err.what() << std::endl;
		return -1;
	}

	std::cout << "Injected into Spotify!" << std::endl;

	try
	{
		pcDiscord = Carl::injectPayload(agentDir, PIDDiscord, payloadPath);
	}
	catch (const Carl::CarlError& err)
	{
		std::cout << "ERROR Injecting into Discord: " << err.what() << std::endl;
		return -1;
	}

	std::cout << "Injected into Discord!" << std::endl;

	std::cout << "PCSpotify port: " << pcSpotify->getPort() << std::endl;
	std::cout << "PCDiscord port: " << pcDiscord->getPort() << std::endl;

	auto queueSpotify = pcSpotify->getQueue();
	auto queueDiscord = pcDiscord->getQueue();

	Connections cons;
	cons.queueSpotify = queueSpotify.get();
	cons.queueDiscord = queueDiscord.get();

	queueSpotify->setRecvCallback(Carl::PT_RENDER_FRAME, RenderFrameCallback, &cons);
	queueSpotify->setRecvCallback(
		Carl::PT_WAVEFORMATEX_REPLY,
		[](EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam) {
			if (nBytesReceived < pack.header.packetSize)
				return;
			pack.buffer->read(wfx);
			receivedWfx = true;
		},
		nullptr
			);

	queueSpotify->push(Carl::PT_START_RENDER_FRAME, EHSN::net::FLAG_PH_NONE, nullptr);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	queueDiscord->push(Carl::PT_START_CAPTURE_FRAME, EHSN::net::FLAG_PH_NONE, nullptr);

	while (queueSpotify->isConnected() && queueDiscord->isConnected())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		/*std::string line;
		std::cout << " >>> ";
		std::getline(std::cin, line);
		auto pBuffer = std::make_shared<EHSN::net::PacketBuffer>(line.size() + 1);
		pBuffer->write(line.c_str(), line.size() + 1, 0);
		queue->push(Carl::PT_ECHO_REQUEST, EHSN::net::FLAG_PH_NONE, pBuffer);*/
	}

	return 0;
}