#include "Carl.h"

#include "Carl/CarlError.h"
#include "Carl/AgentRunner.h"
#include "Carl/ProcessArchitecture.h"

namespace Carl
{
	PayloadConnectorRef injectPayload(const std::string& agentDir, ProcessID targetPID, const std::string& payloadPath)
	{
		auto procArch = getProcArch(targetPID);

		auto procArchStr = ProcArchToStr(procArch);
		size_t offset = payloadPath.find("[.]");
		if (offset == std::string::npos)
			throw CarlError("Invalid payloadPath! Unable to find architecture sequence '[.]'!");
		std::string correctedPayloadPath = payloadPath;
		correctedPayloadPath[offset + 0] = procArchStr[0];
		correctedPayloadPath[offset + 1] = procArchStr[1];
		correctedPayloadPath[offset + 2] = procArchStr[2];

		auto ppc = std::make_shared<PayloadConnector>();

		runAgent(agentDir, procArch, targetPID, correctedPayloadPath, "connectToHost", std::to_string(ppc->getPort()));

		ppc->accept();

		return ppc;
	}
}