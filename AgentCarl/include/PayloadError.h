#pragma once

#include <stdexcept>

namespace AgentCarl
{
	class PayloadError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
}