#pragma once

#include "Carl/CarlError.h"
#include "Carl/Types.h"
#include "Carl/Defines.h"
#include "Carl/PayloadConnector.h"

namespace Carl
{
	CARL_API PayloadConnectorRef injectPayload(const std::string& agentDir, ProcessID targetPID, const std::string& payloadPath);
}