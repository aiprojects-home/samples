#pragma once

#include "auxLZWHash.h"

namespace aux
{

	class LZWCore
	{
	private:

		unsigned char m_AccByte;       // ���� ��� ���������� ����� ��� �������� �����
		unsigned char m_nAccFreeBits;  // ����� ��������� ����� 

		LZWHash  m_HashTable;          // ���-������� ��� �������� / ����������

	public:

		LZWCore()
		{

		}

		void Init(unsigned char maxbit = 12)
		{
			m_AccByte = 0;
			m_nAccFreeBits = 8;

			m_HashTable.Init(maxbit);
			m_HashTable.Clear();
		}

		// ��������� �� ��������� LZW � �������������� ����������.
		template <typename T1, typename T2, typename T3> void Encode(T1& it_begin, T2& it_end, T3& it_out)
		{
			unsigned __int32 curPrefix{ LZW_EMPTY_PREFIX };
			unsigned __int32 curCode{ 0 };
			unsigned char    curSymbol{ 0 };
			unsigned char    curCodeLength{ LZW_START_CODE_LENGTH };

			std::pair<unsigned __int32, unsigned char> lzw_string;

			m_HashTable.Clear();

			while (it_begin != it_end)
			{
				curSymbol = *it_begin;
				it_begin++;

				// ��������� ������ �� �������� � ���������� �������:
				lzw_string = std::make_pair(curPrefix, curSymbol);

				// ���������, ���� �� ����� ������ � �������:
				if (auto r = m_HashTable.HasString(lzw_string); r.has_value())
				{
					// ��, ����� ������ ����. ������� ���������� �� �����.
					curPrefix = r.value().first;
				}
				else
				{
					// ����� ������ ���. ���������� ��� ����������� ������ � ����� ������.
					PutCode(it_out, curPrefix, curCodeLength);

					// ����� ������, ������� ��������� ����� ������ ���������� ���������:
					curPrefix = static_cast<unsigned __int32>(curSymbol);

					// ��������� � ������� ����� ���������� ������:
					if (auto r = m_HashTable.AddString(lzw_string); r.has_value())
					{
						// ����� ������ ���������. ��������, ����� ���������� ����������� ����� ����.
						curCodeLength = r.value().second;
					}
					else
					{
						// ������ �� ����������. ��������, ������� �����������. ����� ��� ������� � �������� �������.
						PutCode(it_out, LZW_CODE_CLEAR, curCodeLength);
						m_HashTable.Clear();
						curCodeLength = LZW_START_CODE_LENGTH;

					}
				}
			}

			// ������ ���������. ����� ��������� ������� (���������� ������) � ��� ����� ������ � �������������.
			PutCode(it_out, curPrefix, curCodeLength);
			PutCode(it_out, LZW_CODE_END, curCodeLength, true);

		}

		// ������������� �� ��������� LZW � �������������� ����������.
		template <typename T1, typename T2, typename T3> void Decode(T1& it_begin, T2& it_end, T3& it_out)
		{
			unsigned __int32 lzwOldCode{ LZW_EMPTY_CODE };
			unsigned __int32 lzwNewCode{ LZW_EMPTY_CODE };
			unsigned char    curCodeLength{ LZW_START_CODE_LENGTH };

			std::vector<unsigned char> lzwString;
			unsigned char lzwFirstChar{ 0 };

			m_HashTable.Clear();

			for (;;)
			{
				lzwNewCode = GetCode(it_begin, curCodeLength);

				if (lzwNewCode == LZW_CODE_END)
				{
					// ����� ������.
					return;
				}
				else

					if (lzwNewCode == LZW_CODE_CLEAR)
					{
						// ������� �������, ��� �������� �������.
						m_HashTable.Clear();
						curCodeLength = LZW_START_CODE_LENGTH;
						lzwOldCode = LZW_EMPTY_CODE;
						lzwNewCode = LZW_EMPTY_CODE;

						continue;
					}
					else

						if (lzwOldCode == LZW_EMPTY_CODE)
						{
							// ��� ������ ���, ��������� ������� ���� ���. �� �������� ������ � ��� ���� �������.
							(*it_out) = static_cast<unsigned char>(lzwNewCode);
							it_out++;

							lzwOldCode = lzwNewCode;

							continue;
						}

				// ����� ������������ ������, ����� "������" ��� ��� ����.

				if (m_HashTable.HasCode(lzwNewCode))
				{
					// ����� ��� ���� � �������. �������� ������ ����� ����.
					m_HashTable.GetString(lzwNewCode, lzwString);

				}
				else
				{
					// ������ ���� � ������� ���.

					// �������� ������ ��� "�������" ����. � ��������� � �� ����� ������ ������.
					m_HashTable.GetString(lzwOldCode, lzwString);
					lzwString.push_back(lzwString[0]);
				}

				// ������� ������ � ����� � ��������� �� � ������� �����.

				std::copy(lzwString.begin(), lzwString.end(), it_out);

				m_HashTable.AddString(std::make_pair(lzwOldCode, lzwString[0]));

				// �������� ����� ���������� ����:
				curCodeLength = m_HashTable.GetNextCodeLength();

				lzwOldCode = lzwNewCode;

			}
		}

	private:

		// ��������� ��� (c) �������� ����� (l) � �������������� ��������� ������. 
		// ���� flush == true - ��� ������ �� �����-���������� ������������� (��� ���������� ����).
		template <typename T> void PutCode(T& out_it, unsigned __int32 c, unsigned char l, bool flush = false)
		{
			unsigned __int32 code{ c };
			unsigned char code_remain{ l };

			while (code_remain)
			{
				unsigned char bits = std::min(code_remain, m_nAccFreeBits);

				m_AccByte |= (code & ((1 << bits) - 1)) << (8 - m_nAccFreeBits);

				code >>= bits;
				code_remain -= bits;
				m_nAccFreeBits -= bits;

				if (!m_nAccFreeBits)
				{
					*out_it = m_AccByte;
					out_it++;
					m_AccByte = 0;
					m_nAccFreeBits = 8;
				}
			}

			if ((flush) && (m_nAccFreeBits != 8))
			{
				*out_it = m_AccByte;
				out_it++;
				m_AccByte = 0;
				m_nAccFreeBits = 8;
			}

		}

		// �������� ��� �������� ����� (l) �� ��������� �����.
		template <typename T> unsigned __int32 GetCode(T& in_it, unsigned char l)
		{
			unsigned __int32 code{ 0 };
			unsigned char code_remain{ l };

			while (code_remain)
			{
				if (m_nAccFreeBits == 8)
				{
					m_AccByte = *in_it;
					in_it++;
					m_nAccFreeBits = 0;
				}

				unsigned char bits = std::min(code_remain, static_cast<unsigned char>(8 - m_nAccFreeBits));

				code |= (m_AccByte & ((1 << bits) - 1)) << (l - code_remain);

				m_AccByte >>= bits;
				m_nAccFreeBits += bits;
				code_remain -= bits;
			}

			return code;
		}
	};
}