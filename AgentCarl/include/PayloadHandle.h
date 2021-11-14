#pragma once

#include <memory>
#include <map>
#include <string>

#include "Types.h"

namespace AgentCarl
{
	class PayloadHandle;

	typedef std::shared_ptr<PayloadHandle> PayloadHandleRef;

	class PayloadHandle
	{
	public:
		virtual ~PayloadHandle() noexcept(false);
	public:
		static PayloadHandleRef create(const std::string& payloadPath, ProcessID processID, PayloadID payloadID);
	public:
		bool inject();
		bool detach();
		PayloadID getID() const;
		operator bool() const;
	public:
		virtual bool call(const std::string& funcName, const void* param, uint64_t paramSize) = 0;
	protected:
		virtual bool injectImpl() = 0;
		virtual bool detachImpl() = 0;
	protected:
		PayloadHandle(const std::string& payloadPath, ProcessID processID, PayloadID payloadID);
	protected:
		PayloadID m_payloadID;
		std::string m_payloadPath;
		ProcessID m_processID;
		bool m_isInjected = false;
	};

	typedef std::map<PayloadID, PayloadHandleRef> _PayloadMap;
}