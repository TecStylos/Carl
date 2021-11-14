#pragma once

#include <string>
#include <map>

namespace AgentCarl {
	#if defined AGENT_CARL_PLATFORM_WINDOWS
	typedef unsigned long ProcessID;
	#elif defined AGENT_CARL_PLATFORM_UNIX
	typedef int ProcessID;
	#endif

	typedef uint32_t PayloadID;
} // namespace PluS