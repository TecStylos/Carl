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
		static PayloadHandleRef create(const std::string& payloadPath, ProcessID processID);
	public:
		void inject();
		operator bool() const;
	public:
		virtual void call(const std::string& funcName, const void* param, uint64_t paramSize) = 0;
	protected:
		virtual void injectImpl() = 0;
	protected:
		PayloadHandle(const std::string& payloadPath, ProcessID processID);
	protected:
		std::string m_payloadPath;
		ProcessID m_processID;
		bool m_isInjected = false;
	};
}