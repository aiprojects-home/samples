#include "auxLZWHash.h"

namespace aux
{
	LZWHash::LZWHash()
	{

	}

	void LZWHash::Init(unsigned char maxbit)
	{
		m_nMaxCodeLength = maxbit;
	}

	void LZWHash::Clear()
	{
		m_nCurrentCodeLength = LZW_START_CODE_LENGTH;
		m_nCurrentCodeValue = LZW_CODE_FIRST;
		m_nMaxCurrentValue = ((1 << m_nCurrentCodeLength) - 1);

		m_bIsLocked = false;

		// Заполняем таблицы строк корневыми значениями.

		m_CodeToStringMap.clear();
		m_StringToCodeMap.clear();

		for (unsigned int i = 0; i < 0x100; i++)
		{
			m_CodeToStringMap[i] = std::make_pair(LZW_EMPTY_PREFIX, i);
			m_StringToCodeMap[std::make_pair(LZW_EMPTY_PREFIX, i)] = i;
		}
	}

	std::optional<std::pair<unsigned __int32, unsigned char>> LZWHash::HasString(const std::pair<unsigned __int32, unsigned char> &s)
	{
		auto r = m_StringToCodeMap.find(s);

		if (r == std::end(m_StringToCodeMap))
		{
			return {};
		};

		// Возвращаем пару "код - длина".

		return std::make_pair((*r).second, m_nCurrentCodeLength);
	}

	std::optional<std::pair<unsigned __int32, unsigned char>> LZWHash::AddString(const std::pair<unsigned __int32, unsigned char> & s)
	{
		if (m_bIsLocked)
		{
			// Таблица уже переполнена.
			return {};
		}

		m_CodeToStringMap[m_nCurrentCodeValue] = s;
		m_StringToCodeMap[s] = m_nCurrentCodeValue;

		// Сохраняем пару для возврата - код и его длина.
		std::pair<unsigned __int32, unsigned char> r = std::make_pair(m_nCurrentCodeValue++, m_nCurrentCodeLength);

		// Проверяем, не нужно ли увеличить длину кода.
		if (m_nCurrentCodeValue > m_nMaxCurrentValue)
		{
			m_nCurrentCodeLength++;
			m_nMaxCurrentValue = ((1 << m_nCurrentCodeLength) - 1);

			if (m_nCurrentCodeLength > m_nMaxCodeLength)
			{
				// Таблица переполнена. Блокируем ее.
				m_bIsLocked = true;

				// Однако, оставляем старую длину кода и ограничивающее значение.
				m_nCurrentCodeLength = m_nMaxCodeLength;
				m_nMaxCurrentValue = ((1 << m_nCurrentCodeLength) - 1);
			}
		}

		return r;
	}

	bool LZWHash::HasCode(unsigned __int32 c)
	{
		if (m_CodeToStringMap.find(c) != std::end(m_CodeToStringMap))
		{
			return true;
		}

		return false;
	}

	unsigned char LZWHash::GetNextCodeLength()
	{
		return m_nCurrentCodeLength;
	}

	bool LZWHash::GetString(unsigned __int32 c, std::vector<unsigned char> &s)
	{
		// Начинаем "разматывать" строку, пока не наткнемся на пустой префикс.

		std::pair<unsigned __int32, unsigned char> lzw_string;
		unsigned __int32 lzw_code{ c };

		s.clear();

		for (;;)
		{
			auto it = m_CodeToStringMap.find(lzw_code);
			if (it == std::end(m_CodeToStringMap))
			{
				// Нет такого кода.
				break;
			}

			auto[prefix, symbol] = (*it).second;

			s.push_back(symbol);

			if (prefix == LZW_EMPTY_PREFIX)
			{
				// Это конец строки.
				std::reverse(s.begin(), s.end());

				return true;
			}

			lzw_code = prefix;
		}

		return false;
	}
}