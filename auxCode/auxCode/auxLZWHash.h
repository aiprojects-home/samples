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

		unsigned char    m_nMaxCodeLength;       // ����. ����� ���� � �����
		unsigned char    m_nCurrentCodeLength;   // ������� ����� ���� � �����
		unsigned __int32 m_nCurrentCodeValue;    // ������� ���
		unsigned __int32 m_nMaxCurrentValue;     // ������������ �������� ��� ������� ����� ����

		bool             m_bIsLocked;            // TRUE - ���� ������� ������������� (�����������)

		std::map<std::pair<unsigned __int32, unsigned char>, unsigned __int32> m_StringToCodeMap;
		std::map<unsigned __int32, std::pair<unsigned __int32, unsigned char>> m_CodeToStringMap;

		LZWHash();

		void Init(unsigned char maxbit = 12);
		void Clear();

		// ���������� ���� (��� ������ - ������� ����� ����) ���� ������ ������, ���� ����� ������ ���.
		std::optional<std::pair<unsigned __int32, unsigned char>> HasString(const std::pair<unsigned __int32, unsigned char> &s);

		// ��������� ������ � ���������� ���� (��� ������ - ����� ����). ���� ������� ����������� - ������ ������.
		std::optional<std::pair<unsigned __int32, unsigned char>> AddString(const std::pair<unsigned __int32, unsigned char> & s);

		// ���������� true, ���� ����� ��� ���� � ������� �����.
		bool HasCode(unsigned __int32 c);

		// ���������� ����� ���������� ����:
		unsigned char GetNextCodeLength();

		// ���������� ������ s ��� ���� c. true/false - ����� ��������.
		bool GetString(unsigned __int32 c, std::vector<unsigned char> &s);

	};
};