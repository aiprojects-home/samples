#pragma once

#include "XGlobals.h"

class XAux
{
public:
	static void HGLOBAL_deleter(HGLOBAL* hMem);
	using HGLOBAL_deleter_type = decltype(&XAux::HGLOBAL_deleter);

	static void COM_deleter(IUnknown* pInterface);
	using COM_deleter_type = decltype(&XAux::COM_deleter);

	// Функция форматирует строку и возвращает объект std::wstring.
	template<typename ... TS>
	static std::wstring FormatString(const wchar_t* pFormat, TS ... args)
	{
		int nBufferLength = (_snwprintf(nullptr, 0, pFormat, args ...) + 1);

		if (nBufferLength <= 0)
		{
			throw std::exception("XAux::FormatString(): _snwprintf() error.");
		}

		size_t nBufferSize = static_cast<size_t>(nBufferLength);

		std::unique_ptr<wchar_t[]> upBuffer(new wchar_t[nBufferSize]);

		_snwprintf(upBuffer.get(), nBufferSize, pFormat, args ...);

		return std::wstring(upBuffer.get(), upBuffer.get() + nBufferSize - 1);
	}

	// Функция форматирует строку и отправляет ее содержимое в окно отладочной информации.
	template<typename ... TS>
	static void PrintDebug(const wchar_t* pFormat, TS ... args)
	{
		std::wstring s = FormatString(pFormat, args...);

		OutputDebugStringW(s.c_str());
	}

	// Функция очищает адаптеры контейнеров (например, очереди и стеки).
	template<typename T>
	static void ClearAdapter(T& c)
	{
		T empty;

		std::swap(c, empty);
	};
};

