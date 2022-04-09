#include "pch.h"
#include "XEFMReader.h"
#include "XEFM.h"
#include "XException.h"

XEFMReader::XEFMReader()
{

}

void XEFMReader::Open(const wchar_t* pFileName)
{
	std::unique_lock lock{ m_Lock };

	try
	{
		XEFM::Current().GetReaderInfo(pFileName, m_FileName, m_MinPos, m_MaxPos);

		m_Stream.open(m_FileName, std::ios::in | std::ios::binary);

		if (!m_Stream.is_open())
		{
			throw XException(L"XEFMReader::Open(): can't open file '%s'", pFileName);
		}

		m_CurPos = m_MinPos;
		m_Stream.seekg(m_CurPos);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XEFMReader::Open(): can't open reader for '%s'", pFileName);
	}
}

void XEFMReader::Close()
{
	std::unique_lock lock{ m_Lock };

	m_Stream.close();
}


bool XEFMReader::IsOpen() const
{
	std::shared_lock lock{ m_Lock };

	return m_Stream.is_open();
}

bool XEFMReader::IsEOF() const
{
	std::shared_lock lock{ m_Lock };

	if (m_Stream.is_open())
	{
		return (m_CurPos > m_MaxPos);
	}
	else
	{
		// Если не открыт -- возвращаем EOF.
		return true;
	};
}

bool XEFMReader::ReadLine(std::string& s)
{
	std::unique_lock lock{ m_Lock };

	if ((!m_Stream.is_open()) || (m_CurPos > m_MaxPos))
	{
		return false;
	}

	std::forward_list<char> listChars;

	// '\r\n\' - признак конца строки, сохраняем все пробелы
	for (char c;;)
	{
		m_Stream.get(c);
		m_CurPos++;

		if (c == '\r')
		{
			// Конец строки. Возможно, следующий символ - '\n'. Проверяем.
			if (m_CurPos <= m_MaxPos)
			{
				m_Stream.get(c);
				m_CurPos++;

				if (c != '\n')
				{
					// Нет, не '\n'. Возвращаем назад в поток.
					m_Stream.unget();
					m_CurPos--;
				};

				break;
			} 
			else
			{
				// Последний символ, выходим.
				break;
			}
		}
		else
		if (c == '\n')
		{
			// Конец строки.
			break;

		} else
		{
			// Еще один символ, сохраняем его.
			listChars.push_front(c);
		}

		if (m_CurPos > m_MaxPos)
		{
			// EOF.
			break;
		};
	}

	listChars.reverse();
	s.assign(listChars.begin(), listChars.end());

	return true;
}

uint32_t XEFMReader::ReadBytes(char* pBuffer, const uint32_t nCount)
{
	std::unique_lock lock{ m_Lock };

	if ((!m_Stream.is_open()) || (m_CurPos > m_MaxPos))
	{
		return 0;
	}

	uint32_t bytes_allowed = min(nCount, (m_MaxPos - m_CurPos + 1));
	uint32_t bytes_read{ 0 };

	m_Stream.read(pBuffer, bytes_allowed);
	bytes_read = (uint32_t)m_Stream.gcount();

	m_CurPos += bytes_read;

	return bytes_read;
}

uint32_t XEFMReader::Tell() const
{
	std::shared_lock lock{ m_Lock };

	if (!m_Stream.is_open())
	{
		return 0;
	}

	// Возвращаем индекс, где 0 - начало.

	return (m_CurPos - m_MinPos);
}

bool XEFMReader::Seek(const uint32_t nPos, XSEEK_TYPE seektype)
{
	std::unique_lock lock{ m_Lock };

	if (!m_Stream.is_open())
	{
		return false;
	}

	uint32_t nNewPos;

	// Вычисляем смещение в зависимости от типа позиционирования.
	switch (seektype)
	{
	    case XSEEK_TYPE::XSEEK_BEGIN:
	    {
			// От начала.
			nNewPos = nPos + m_MinPos;
    		break;
	    }
	    case XSEEK_TYPE::XSEEK_CURRENT:
	    {
			// От текущей позиции.
			nNewPos = nPos + m_CurPos;
    		break;
    	}
		case XSEEK_TYPE::XSEEK_END:
		{
			// От конца.
			nNewPos = m_MaxPos - nPos;
			break;
		}
    	default:
    	{
			// Неверный тип.
			return false;
	    	break;
	    }
	};

	// Позиция должна быть в допустимых пределах.
	if ((nNewPos > m_MaxPos) || (nNewPos < m_MinPos))
	{
		return false;
	}

	// Устанавливаем потоковый указатель.
	m_Stream.seekg(nNewPos);
	m_CurPos = nNewPos;

	return true;
}

uint32_t XEFMReader::GetSize() const
{
	std::shared_lock lock{ m_Lock };

	if (!m_Stream.is_open())
	{
		return 0;
	}

	return (m_MaxPos - m_MinPos + 1);
}
