#include "PayloadHandle.h"

#if defined AGENT_CARL_PLATFORM_WINDOWS
#include "Platform/Win32PayloadHandle.h"
#elif defined AGENT_CARL_PLATFORM_UNIX
#error Unix currently unsupported!
#endif

#include <stdexcept>
#include <filesystem>

namespace AgentCarl
{
	PayloadHandle::~PayloadHandle() noexcept(false)
	{}

	PayloadHandleRef PayloadHandle::create(const std::string& payloadPath, ProcessID processID)
	{
		#if defined AGENT_CARL_PLATFORM_WINDOWS
		return std::make_shared<Win32PayloadHandle>(payloadPath, processID);
		#elif defined AGENT_CARL_PLATFORM_UNIX
		#error Unix currently unsupported!
		#endif
	}

	void PayloadHandle::inject()
	{
		if (m_isInjected)
			return;

		injectImpl();

		m_isInjected = true;
	}

	PayloadHandle::operator bool() const
	{
		return m_isInjected;
	}

	PayloadHandle::PayloadHandle(const std::string& payloadPath, ProcessID processID)
		: m_payloadPath(std::filesystem::absolute(payloadPath).string()), m_processID(processID)
	{}
}