#include "Carl/PayloadConnector.h"

#include <condition_variable>

#include "Carl/CarlError.h"

namespace Carl
{
	void _internalSessionInitFunc(EHSN::net::SecSocketRef sock, void* pParam)
	{
		auto& pc = *(PayloadConnector*)pParam;
		pc.m_queue = std::make_shared<EHSN::net::ManagedSocket>(sock);

		{
			std::unique_lock lock(pc.m_mtx);
			pc.m_isAccepted = true;
		}
		pc.m_conVar.notify_all();
	}

	PayloadConnector::PayloadConnector()
		: m_acceptor("0", _internalSessionInitFunc, this, nullptr, EHSN::crypto::defaultRDG, 4096)
	{}

	void PayloadConnector::accept()
	{
		m_isAccepted = false;
		m_acceptor.newSession(true);
		std::unique_lock lock(m_mtx);
		m_conVar.wait(lock, [this] { return m_isAccepted; });
	}

	uint16_t PayloadConnector::getPort() const
	{
		return m_acceptor.getPort();
	}

	EHSN::net::ManagedSocketRef PayloadConnector::getQueue()
	{
		return m_queue;
	}
}