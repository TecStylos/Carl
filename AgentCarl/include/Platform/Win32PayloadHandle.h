#pragma once

#include <Windows.h>

#include "../PayloadHandle.h"

#include "Platform/Win32HandleHelper.h"

namespace AgentCarl
{
	class Win32PayloadHandle : public PayloadHandle
	{
	public:
		Win32PayloadHandle(const std::string& payloadPath, ProcessID processID);
	public:
		virtual void call(const std::string& funcName, const void* param, uint64_t paramSize) override;
	protected:
		virtual void injectImpl() override;
	private:
		HMODULE getPayloadBaseAddr() const;
		LPTHREAD_START_ROUTINE getPayloadFuncAddr(const std::string& funcName) const;
	private:
		void* targetAlloc(uint64_t size);
		void targetFree(void* base);
		void targetWrite(void* destTarget, const void* src, uint64_t size);
		void targetRead(void* dest, const void* srcTarget, uint64_t size);
	private:
		HMODULE m_payloadBase = nullptr;
		Win32HandleHelper m_hProcTarget = 0;
	private:
		class MemoryHelper
		{
		public:
			MemoryHelper() = default;
			MemoryHelper(void* ptr, Win32PayloadHandle* pHandle) : m_ptr(ptr), m_pHandle(pHandle) {}
			MemoryHelper(MemoryHelper&& other) { *this = std::move(other); }
			MemoryHelper(const MemoryHelper&) = delete;
		public:
			operator bool() { return m_ptr && m_pHandle; }
			void* operator*() { return m_ptr; }
			const void* operator*() const { return m_ptr; }
			MemoryHelper& operator=(MemoryHelper&& other) { autoFree(); std::swap(m_ptr, other.m_ptr); std::swap(m_pHandle, other.m_pHandle); return *this; }
		private:
			void autoFree() { if (!!*this) m_pHandle->targetFree(m_ptr); m_ptr = nullptr; m_pHandle = nullptr; }
		private:
			void* m_ptr = nullptr;
			Win32PayloadHandle* m_pHandle = nullptr;
		};
	};
}