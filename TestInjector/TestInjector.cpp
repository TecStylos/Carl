#include <iostream>

#include <Carl.h>
#include <CarlPayload.h>
#include <set>

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
	catch (Carl::CarlError& err)
	{
		std::cout << "ERROR: " << err.what() << std::endl;
		return -1;
	}

	std::cout << "PayloadConnector port: " << pc->getPort() << std::endl;

	auto queue = pc->getQueue();

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