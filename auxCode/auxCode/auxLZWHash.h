#pragma once

#include <map>
#include <vector>
#include <optional>

namespace aux
{
	constexpr unsigned __int32 LZW_EMPTY_PREFIX = 0xFFFFFFFF;
	constexpr unsigned char    LZW_START_CODE_LENGTH = 0x9;

	constexpr unsigned __int32 LZW_EMPTY_CODE = 0xFFFFFFFF;
	constexpr unsigned __int32 LZW_CODE_CLEAR = 0x100;
	constexpr unsigned __int32 LZW_CODE_END   = 0x101;
	constexpr unsigned __int32 LZW_CODE_FIRST = 0x102;

	class LZWHash
	{
		friend class LZWCore;

	private:

		unsigned char    m_nMaxCodeLength;       // макс. длина кода в битах
		unsigned char    m_nCurrentCodeLength;   // текуща€ длина кода в битах
		unsigned __int32 m_nCurrentCodeValue;    // текущий код
		unsigned __int32 m_nMaxCurrentValue;     // максимальное значение дл€ текущей длины кода

		bool             m_bIsLocked;            // TRUE - если таблица заблокирована (переполнена)

		std::map<std::pair<unsigned __int32, unsigned char>, unsigned __int32> m_StringToCodeMap;
		std::map<unsigned __int32, std::pair<unsigned __int32, unsigned char>> m_CodeToStringMap;

		LZWHash();

		void Init(unsigned char maxbit = 12);
		void Clear();

		// ¬озвращает пару (код строки - текуща€ длина кода) либо пустой объект, если такой строки нет.
		std::optional<std::pair<unsigned __int32, unsigned char>> HasString(const std::pair<unsigned __int32, unsigned char> &s);

		// ƒобавл€ет строку и возвращает пару (код строки - длина кода). ≈сли таблица переполнена - пустой объект.
		std::optional<std::pair<unsigned __int32, unsigned char>> AddString(const std::pair<unsigned __int32, unsigned char> & s);

		// ¬озвращает true, если такой код есть в таблице кодов.
		bool HasCode(unsigned __int32 c);

		// ¬озвращает длину следующего кода:
		unsigned char GetNextCodeLength();

		// ¬озвращает строку s дл€ кода c. true/false - успех операции.
		bool GetString(unsigned __int32 c, std::vector<unsigned char> &s);

	};
};