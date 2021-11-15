#pragma once

#include <stdexcept>

namespace Carl
{
	class CarlError : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
}