#include "Carl/ProcessArchitecture.h"

#if defined CARL_PLATFORM_WINDOWS
#include "Carl/Platform/Win32ProcessArchitecture.h"
#elif defined CARL_PLATFORM_UNIX
#include "Carl/Platform/UnixProcessArchitecture.h"
return UnixGetProcArch(pid);
#endif

namespace Carl
{
	ProcArch getProcArch(ProcessID pid)
	{
		#if defined CARL_PLATFORM_WINDOWS
		return Win32GetProcArch(pid);
		#elif defined CARL_PLATFORM_UNIX
		return UnixGetProcArch(pid);
		#endif
	}

	std::string ProcArchToStr(ProcArch procArch)
	{
		switch (procArch)
		{
		case ProcArch::x64:
			return "x64";
		case ProcArch::x86:
			return "x86";
		}
		return "unknown";
	}
}