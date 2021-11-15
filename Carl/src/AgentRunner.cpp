#include "Carl/AgentRunner.h"

#if defined CARL_PLATFORM_WINDOWS
#include "Carl/Platform/Win32AgentRunner.h"
#elif defined CARL_PLATFORM_UNIX
#include "Carl/Platform/UnixAgentRunner.h"
#endif

#include <filesystem>

namespace Carl
{
	void runAgent(const std::string& agentDir, ProcArch procArch, ProcessID targetPID, const std::string& payloadPath, const std::string& funcName, const std::string& message)
	{
		auto agentPath = std::filesystem::path(agentDir).append("AgentCarl_" + ProcArchToStr(procArch));
		agentPath.make_preferred();

		std::string agentExeNoExt = agentPath.string();
		std::string strTargetPID = std::to_string(targetPID);
		std::string quotedPayloadPath = "\"" + payloadPath + "\"";
		std::string quotedMessage = "\"" + message + "\"";
		std::string cmdLine = "AgentCarl " + strTargetPID + " " + quotedPayloadPath + " " + funcName + " " + quotedMessage;

		#if defined CARL_PLATFORM_WINDOWS
		return Win32RunAgent(agentExeNoExt + ".exe", cmdLine);
		#elif defined CARL_PLATFORM_UNIX
		return UnixRunAgent(agentExeNoExt, cmdLine);
		#endif
	}
}