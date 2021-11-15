#include <iostream>

#include "Carl.h"

int main()
{
	std::string agentDir = "../AgentCarl";

	Carl::ProcessID targetPID = 21612;
	std::string payloadPath = "..\\TestPayload\\TestPayload.dll";

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