#pragma once

namespace Carl
{
	#if defined CARL_PLATFORM_WINDOWS
	typedef unsigned long ProcessID;
	#elif defined CARL_PLATFORM_UNIX
	typedef int ProcessID;
	#endif
} // namespace Carl