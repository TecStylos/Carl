#pragma once

#include <memory>
#include <string>

#include <EHSN.h>

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
		void sessionFunc(EHSN::net::SecSocketRef sock, void* pParam);
	private:
		std::shared_ptr<EHSN::net::ManagedSocket> m_queue;
		EHSN::net::SecAcceptor m_acceptor;
	private:
		std::mutex m_mtx;
		std::condition_variable m_conVar;
		bool m_isAccepted = false;
	private:
		friend void _internalSessionInitFunc(EHSN::net::SecSocketRef sock, void* pParam);
	};

	typedef std::shared_ptr<PayloadConnector> PayloadConnectorRef;
}