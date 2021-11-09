#pragma once

#include "auxLZWHash.h"

namespace aux
{

	class LZWCore
	{
	private:

		unsigned char m_AccByte;       // байт для накопления битов при упаковке кодов
		unsigned char m_nAccFreeBits;  // число свободних битов 

		LZWHash  m_HashTable;          // хеш-таблица для упаковки / распаковки

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

		// Кодировка по алгоритму LZW с использованием итераторов.
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

				// Формируем строку из префикса и считанного символа:
				lzw_string = std::make_pair(curPrefix, curSymbol);

				// Проверяем, есть ли такая строка в таблице:
				if (auto r = m_HashTable.HasString(lzw_string); r.has_value())
				{
					// Да, такая строка есть. Префикс становится ее кодом.
					curPrefix = r.value().first;
				}
				else
				{
					// Такой строки нет. Сбрасываем уже накопленную строку в поток вывода.
					PutCode(it_out, curPrefix, curCodeLength);

					// Новый символ, который образовал новую строку становится префиксом:
					curPrefix = static_cast<unsigned __int32>(curSymbol);

					// Добавляем в таблицу новую уникальную строку:
					if (auto r = m_HashTable.AddString(lzw_string); r.has_value())
					{
						// Новая строка добавлена. Возможно, после добавления увеличилась длина кода.
						curCodeLength = r.value().second;
					}
					else
					{
						// Строка не добавилась. Вероятно, таблица переполнена. Пишем код очистки и начинаем сначала.
						PutCode(it_out, LZW_CODE_CLEAR, curCodeLength);
						m_HashTable.Clear();
						curCodeLength = LZW_START_CODE_LENGTH;

					}
				}
			}

			// Данные кончились. Пишем последний префикс (сбрасываем строку) и код конца данных с выталкиванием.
			PutCode(it_out, curPrefix, curCodeLength);
			PutCode(it_out, LZW_CODE_END, curCodeLength, true);

		}

		// Декодирование по алгоритму LZW с использованием итераторов.
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
					// Конец данных.
					return;
				}
				else

					if (lzwNewCode == LZW_CODE_CLEAR)
					{
						// Очистка таблицы, все начнется сначала.
						m_HashTable.Clear();
						curCodeLength = LZW_START_CODE_LENGTH;
						lzwOldCode = LZW_EMPTY_CODE;
						lzwNewCode = LZW_EMPTY_CODE;

						continue;
					}
					else

						if (lzwOldCode == LZW_EMPTY_CODE)
						{
							// Это первый код, поскольку старого кода нет. Он является корнем и его надо вывести.
							(*it_out) = static_cast<unsigned char>(lzwNewCode);
							it_out++;

							lzwOldCode = lzwNewCode;

							continue;
						}

				// Здесь обрабатываем случай, когда "старый" код уже есть.

				if (m_HashTable.HasCode(lzwNewCode))
				{
					// Такой код есть в таблице. Получаем строку этого кода.
					m_HashTable.GetString(lzwNewCode, lzwString);

				}
				else
				{
					// Такого кода в таблице нет.

					// Получаем строку для "старого" кода. И добавляем в ее конец первый символ.
					m_HashTable.GetString(lzwOldCode, lzwString);
					lzwString.push_back(lzwString[0]);
				}

				// Выводим строку в поток и добавляем ее в таблицу строк.

				std::copy(lzwString.begin(), lzwString.end(), it_out);

				m_HashTable.AddString(std::make_pair(lzwOldCode, lzwString[0]));

				// Получаем длину следующего кода:
				curCodeLength = m_HashTable.GetNextCodeLength();

				lzwOldCode = lzwNewCode;

			}
		}

	private:

		// Сохраняет код (c) заданной длины (l) с использованием итератора вывода. 
		// Если flush == true - все данные из байта-накопителя выталкиваются (для последнего кода).
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

		// Получает код заданной длины (l) из итератора ввода.
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