#include "Carl.h"

#include "Carl/CarlError.h"
#include "Carl/AgentRunner.h"
#include "Carl/ProcessArchitecture.h"

namespace Carl
{
	PayloadConnectorRef injectPayload(const std::string& agentDir, ProcessID targetPID, const std::string& payloadPath)
	{
		auto procArch = getProcArch(targetPID);

		auto ppc = std::make_shared<PayloadConnector>();

		runAgent(agentDir, procArch, targetPID, payloadPath, "connectToHost", std::to_string(ppc->getPort()));

		ppc->accept();

		return ppc;
	}
}