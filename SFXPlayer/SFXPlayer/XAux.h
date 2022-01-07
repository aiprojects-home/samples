#pragma once

#include "framework.h"
#include <string>

class XAux
{
public:
	static void HGLOBAL_deleter(HGLOBAL* hMem);
	using HGLOBAL_deleter_type = decltype(&XAux::HGLOBAL_deleter);

	static void COM_deleter(IUnknown* pInterface);
	using COM_deleter_type = decltype(&XAux::COM_deleter);

	// Function formats string and returns std::wstring object.
	template<typename ... TS>
	static std::wstring FormatString(const wchar_t* pFormat, TS ... args)
	{
		int nBufferLength = (_snwprintf(nullptr, 0, pFormat, args ...) + 1);

		if (nBufferLength <= 0)
		{
			throw std::exception("XAux::FormatString(): _snwprintf() error.");
		}

		size_t nBufferSize = static_cast<size_t>(nBufferLength);

		std::unique_ptr<wchar_t[]> spBuffer(new wchar_t[nBufferSize]);

		_snwprintf(spBuffer.get(), nBufferSize, pFormat, args ...);

		return std::wstring(spBuffer.get(), spBuffer.get() + nBufferSize - 1);
	}

	// Function formats string and sends it's contents to debug output stream.
	template<typename ... TS>
	static void PrintDebug(const wchar_t* pFormat, TS ... args)
	{
		std::wstring s = FormatString(pFormat, args...);

		OutputDebugStringW(s.c_str());
	}

	// Function clears container adapters (e.g. queues).
	template<typename T>
	static void ClearAdapter(T& c)
	{
		T empty;

		std::swap(c, empty);
	};
};

