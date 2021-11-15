#pragma once

#include <string>

#include "Types.h"
#include "Carl/ProcessArchitecture.h"

namespace Carl
{
	void runAgent(const std::string& agentDir, ProcArch procArch, ProcessID targetPID, const std::string& payloadPath, const std::string& funcName, const std::string& message);
}