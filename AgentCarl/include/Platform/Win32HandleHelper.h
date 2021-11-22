#pragma once

#include <Windows.h>

#include <algorithm>

namespace AgentCarl
{
	class Win32HandleHelper
	{
	public:
		Win32HandleHelper() = default;
		Win32HandleHelper(Win32HandleHelper&& other) { internalClose(); std::swap(m_handle, other.m_handle); };
		Win32HandleHelper(HANDLE handle) : m_handle(handle) {}
		~Win32HandleHelper() { internalClose(); }
	private:
		void internalClose() { if (m_handle) CloseHandle(m_handle); m_handle = nullptr; }
	public:
		HANDLE& operator*() { return m_handle; }
		const HANDLE& operator*() const { return m_handle; }
		Win32HandleHelper& operator=(HANDLE handle) { internalClose(); m_handle = handle; return *this; }
	private:
		HANDLE m_handle = nullptr;
	};
}