#pragma once

#include <memory>
#include <string>

#include "Types.h"
#include "Defines.h"

namespace Carl
{
	class PayloadConnector
	{
	public:
		CARL_API PayloadConnector();
	public:
		CARL_API void accept();
	public:
		CARL_API uint16_t getPort() const;
	private:
		uint16_t m_port = 0;
	};

	typedef std::shared_ptr<PayloadConnector> PayloadConnectorRef;
}