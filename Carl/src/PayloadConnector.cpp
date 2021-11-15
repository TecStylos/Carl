#include "Carl/PayloadConnector.h"

#include "asio.hpp"

namespace Carl
{
	PayloadConnector::PayloadConnector()
	{
		;
	}

	void PayloadConnector::accept()
	{
	}

	uint16_t PayloadConnector::getPort() const
	{
		return m_port;
	}
}