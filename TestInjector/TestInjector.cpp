#include <iostream>

#include "Carl.h"

int main()
{
	std::string agentDir = "..\\..\\..\\bintools\\Debug\\";

	Carl::ProcessID targetPID;
	std::cout << "Target PID: ";
	std::cin >> targetPID;

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

	return 0;
}