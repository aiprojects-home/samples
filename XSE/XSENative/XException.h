#pragma once

#include "XGlobals.h"
#include "XAux.h"

class XException
{
private:
	std::wstring            m_strError;
	std::vector<XException> m_vecInner;

public:
	XException() = delete;

	XException(const XException& other)
	{
		m_strError = other.m_strError;
	}

	XException(XException&& other)
	{
		m_strError = std::move(other.m_strError);
	}

	~XException()
	{

	}

	template<typename ... TS>
	XException(const XException& PrevException, const wchar_t* pFormat, TS ... args)
	{
		m_strError = XAux::FormatString(pFormat, args...);

		// Сохраняем список вложенных исключений.

		m_vecInner.push_back(PrevException);
		m_vecInner.insert(m_vecInner.end(), PrevException.m_vecInner.cbegin(), PrevException.m_vecInner.cend());
	}

	template<typename ... TS>
	XException(const wchar_t* pFormat, TS ... args)
	{
		m_strError = XAux::FormatString(pFormat, args...);
	}

	const wchar_t* GetError() const
	{
		return m_strError.c_str();
	}

	void Dump() const
	{
		::OutputDebugStringW(GetDumpInfo().c_str());
	}

	std::wstring GetDumpInfo() const
	{
		std::wstring strInfo{};

		strInfo += L"DUMP XException(): ";
		strInfo += m_strError.c_str();
		strInfo += L"\n";

		auto print_space = [&strInfo](int count)
		{
			for (int i = 0; i < count; ++i)
			{
				strInfo += L" ";
			}

		};

		for (size_t i = 0; i < m_vecInner.size(); ++i)
		{
			print_space(static_cast<int>(i) + 1);
			strInfo += L"^-- XException(): ";
			strInfo += m_vecInner[i].m_strError.c_str();
			strInfo += L"\n";
		}

		return strInfo;
	}

	const XException& operator = (const XException& e)
	{
		if (&e != this)
		{
			m_strError = e.m_strError;
			m_vecInner = e.m_vecInner;
		}

		return *this;
	}

	XException& operator = (XException &&e)
	{
		if (&e != this)
		{
			m_strError = std::move(e.m_strError);
			m_vecInner = std::move(e.m_vecInner);
		}

		return *this;
	}
};

