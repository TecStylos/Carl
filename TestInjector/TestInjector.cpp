#include <iostream>

#include <Carl.h>
#include <CarlPayload.h>
#include <set>

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
} wbt = WaveBaseType::None;

bool receivedWfx = false;
WAVEFORMATEX wfx;

template <typename T>
bool isFloatingPointBuffer(const void* buffer, uint64_t bufferSize, uint64_t nChannels)
{
	uint64_t sampleSize = nChannels * sizeof(T);
	uint64_t nInRange = 0;
	uint64_t offset = 0;
	while (offset + sampleSize <= bufferSize)
	{
		for (uint64_t i = 0; i < nChannels; ++i)
		{
			float* pTest = (float*)((const char*)buffer + offset + i * sizeof(T));
			if (-1.0f <= *pTest && *pTest <= 1.0f)
				++nInRange;
		}
		offset += sampleSize;
	}

	return nInRange > (bufferSize / sizeof(T) * 9 / 10);
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
		if (isFloatingPointBuffer<float>(buffer, bufferSize, pwfx->nChannels))
			return WaveBaseType::Float;
		return WaveBaseType::Int32;
	case 8:
		if (isFloatingPointBuffer<double>(buffer, bufferSize, pwfx->nChannels))
			return WaveBaseType::Double;
		return WaveBaseType::Int64;
	}

	return WaveBaseType::None;
}

void RenderFrameCallback(EHSN::net::Packet pack, uint64_t nBytesReceived, void* pParam)
{
	if (nBytesReceived < pack.header.packetSize)
		return;

	if (wbt == WaveBaseType::None && receivedWfx)
	{
		wbt = detectWaveBaseType(&wfx, pack.buffer->data(), pack.buffer->size());
		std::cout << "WaveBaseType = " << (int)wbt << std::endl;
	}

	outFileStream.write((const char*)pack.buffer->data(), nBytesReceived);
	outFileStream.flush();
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