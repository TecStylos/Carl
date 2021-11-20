#pragma once

#include <detours.h>

template <typename Function_t>
class CarlDetour
{
public:
	CarlDetour() = default;
	CarlDetour(Function_t realFunc, Function_t detourFunc)
		: m_realFunc(realFunc), m_detourFunc(detourFunc)
	{
		if (NO_ERROR != (m_errCode = DetourTransactionBegin())) return;
		if (NO_ERROR != (DetourUpdateThread(GetCurrentThread()))) return;
		if (NO_ERROR != (DetourAttach(&(PVOID&)m_realFunc, *(PBYTE*)&m_detourFunc))) return;
		if (NO_ERROR != (DetourTransactionCommit())) return;
	}
	~CarlDetour()
	{
		if (!isDetoured())
			return;
		if (NO_ERROR != (DetourTransactionBegin())) return;
		if (NO_ERROR != (DetourUpdateThread(GetCurrentThread()))) return;
		if (NO_ERROR != (DetourDetach(&(PVOID&)m_realFunc, *(PBYTE*)&m_detourFunc))) return;
		if (NO_ERROR != (DetourTransactionCommit())) return;
	}
	CarlDetour(const CarlDetour&) = delete;
	CarlDetour(CarlDetour&& other)
	{
		std::swap(m_realFunc, other.m_realFunc);
		std::swap(m_detourFunc, other.m_detourFunc);
	}
public:
	bool isDetoured() const
	{
		return m_errCode == NO_ERROR && m_realFunc && m_detourFunc;
	}
	Function_t getRealFunc() const
	{
		return m_realFunc;
	}
	Function_t getDetourFunc() const
	{
		return m_detourFunc;
	}
	template <typename Ret, class Class, typename ...Args>
	Ret callReal(Class* pInstance, Args... args) const
	{
		return (pInstance->*getRealFunc())(args...);
	}
private:
	long m_errCode = 0;
	Function_t m_realFunc = nullptr;
	Function_t m_detourFunc = nullptr;
};
template <typename Function_t>
using CarlDetourRef = std::shared_ptr<CarlDetour<Function_t>>;