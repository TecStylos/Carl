#include "Carl/Platform/Win32ErrorMessage.h"

#include <Windows.h>

namespace Carl
{
	std::string Win32LastErrorMessage()
	{
		char* msgBuff;
		DWORD errCode = GetLastError();

		FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			errCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&msgBuff,
			0, NULL
		);

		std::string result = msgBuff;
		LocalFree(msgBuff);

		return result;
	}
}