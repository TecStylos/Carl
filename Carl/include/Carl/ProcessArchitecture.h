#pragma once

#include <string>

#include "Types.h"

namespace Carl
{
	enum class ProcArch
	{
		unknown,
		x86,
		x64
	};

	ProcArch getProcArch(ProcessID pid);

	std::string ProcArchToStr(ProcArch procArch);
}