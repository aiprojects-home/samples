#include <any>
#include <cwctype>

#include "XSoundBankParser.h"
#include "XEFMReader.h"
#include "XException.h"

XTagAttribute::XTagAttribute(std::wstring strName, XTagAttributeType Type, bool bRequired, std::any anyDefaultValue)
{
	m_strAttrName = strName;
	m_Type = Type;
	m_bRequired = bRequired;
	m_anyValue = anyDefaultValue;
	m_bDefined = false;
}

void XTagAttribute::SetValue(std::wstring strValue)
{
	if (m_bDefined)
	{
		throw XException(L"XTagAttribute()::SetValue(): attribute '%s' is already set", m_strAttrName.c_str());
	}

	if (!strValue.size())
	{
		throw XException(L"XTagAttribute()::SetValue(): invalid value for attribute '%s'", m_strAttrName.c_str());
	}

	switch (m_Type)
	{
    	case XTagAttributeType::TYPE_STRING:
    	{
		    size_t nLength{ strValue.size() };

		    if (nLength < 2)
		    {
    			throw XException(L"XTagAttribute()::SetValue(): invalid STRING value for attribute '%s'", m_strAttrName.c_str());
		    };

		    if ((strValue[0] != L'\"') || (strValue[nLength - 1] != L'\"'))
 		    {
    			throw XException(L"XTagAttribute()::SetValue(): no double quotes found in STRING value for attribute '%s'", m_strAttrName.c_str());
	    	}

    		m_anyValue = std::make_any<STRING_TYPE>(strValue.substr(1, nLength - 2));

		    break;
	    };
	    case XTagAttributeType::TYPE_INT:
	    {
    		unsigned long uValue;

    		auto p = strValue.find_first_not_of(L"0123456789");

		    if (p != -1)
		    {
    			throw XException(L"XTagAttribute()::SetValue(): Invalid INT value for attribute '%s'", m_strAttrName.c_str());
		    };

		    try
		    {
    			uValue = std::stoul(strValue);
		    }
		    catch (const std::exception e)
		    {
    			throw XException(L"XTagAttribute()::SetValue(): Value conversion error for INT attribute '%s'", m_strAttrName.c_str());
		    }

		    m_anyValue = std::make_any<INT_TYPE>(uValue);

		    break;
	    };
	    case XTagAttributeType::TYPE_BOOL:
	    {
    		bool bValue;

		    if (strValue == L"TRUE")
		    {
    			bValue = true;
	    	}
    		else
    		if (strValue == L"FALSE")
		    {
   				bValue = false;
		    }
		    else
		    {
   				throw XException(L"XTagAttribute()::SetValue(): Value conversion error for BOOL attribute '%s'", m_strAttrName.c_str());
		    }

		    m_anyValue = std::make_any<BOOL_TYPE>(bValue);

    		break;
    	};
    	case XTagAttributeType::TYPE_FLOAT:
    	{
		    float fValue;

		    auto p = strValue.find_first_not_of(L"0123456789.-");

		    if (p != -1)
		    {
    			throw XException(L"XTagAttribute()::SetValue(): Invalid FLOAT value for attribute '%s'", m_strAttrName.c_str());
		    };

		    try
		    {
    			fValue = std::stof(strValue);
		    }
		    catch (const std::exception e)
		    {
    			throw XException(L"XTagAttribute()::SetValue(): Value conversion error for FLOAT attribute '%s'", m_strAttrName.c_str());
		    }

		    m_anyValue = std::make_any<FLOAT_TYPE>(fValue);

		    break;
	    };
	    default:
	    {
    		throw XException(L"XTagAttribute()::SetValue(): Attribute '%s' has unknown type", m_strAttrName.c_str());
    	}
	}

	m_bDefined = true;

}

bool XTagAttribute::IsDefined()
{
	return m_bDefined;
}

bool XTagAttribute::IsRequired()
{
	return m_bRequired;
}

std::wstring XTagAttribute::GetName()
{
	return m_strAttrName;
}

std::any XTagAttribute::GetValue()
{
	return m_anyValue;
}

XSoundBankEntry::XSoundBankEntry()
{

}

XSoundBankEntry::XSoundBankEntry(XSoundBankEntry&& other)
{
	nId = other.nId;
	strFileName = std::move(other.strFileName);
	spDecoder = std::move(other.spDecoder);
	bFetch = other.bFetch;
	bStream = other.bStream;
	strDescription = std::move(other.strDescription);
}

XSoundBankEntry& XSoundBankEntry::operator = (XSoundBankEntry&& other)
{
	if (this != &other)
	{
		nId = other.nId;
		strFileName = std::move(other.strFileName);
		spDecoder = std::move(other.spDecoder);
		bFetch = other.bFetch;
		bStream = other.bStream;
		strDescription = std::move(other.strDescription);
	}

	return *this;
}

XSoundBankParser::XSoundBankParser()
{
	m_pDestList = nullptr;

	XStringParser::Init(
		{
			XParserElement(std::wregex(L"\\s*<soundbank>\\s*", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr, std::wregex(L"\\s*</soundbank>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr,
			{
				XParserElement(std::wregex(L"\\s*<filelist>\\s*", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr, std::wregex(L"\\s*</filelist>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr,
					{
						XParserElement(std::wregex(L"\\s*<file (.*)\\/>\\s*", std::regex_constants::ECMAScript | std::regex_constants::icase), &FILE_Handler)
					})
			})
		});

}

void XSoundBankParser::Parse(const wchar_t* pFileName, std::list<XSoundBankEntry>* pDest)
{
	XEFMReader reader;

	// Подготавливаем указатель на список для добавления и очищаем набор ID.
	m_pDestList = pDest;
	m_usetId.clear();

	try
	{
		reader.Open(pFileName);
	}
	catch (const XException& e)
	{
		// Can't open bank file.
		throw XException(e, L"XSoundBank::XSoundBankParser::Parse(): can't open file");
	};

	std::string line;
	std::list<std::wstring> listStrings;

	// Reading file lines and puting them into vector.
	while (!reader.IsEOF())
	{
		reader.ReadLine(line);

		std::wstring wline{ line.cbegin(), line.cend() };

		// Двойной слеш -- комментарии до конца строки, удаляем их.
		auto nSlashPos = wline.find(L"//");

		if (nSlashPos != -1)
		{
			wline = wline.substr(0, nSlashPos);
		}

		listStrings.emplace_back(wline);

	};

	try
	{
		XStringParser::Parse(listStrings.begin(), listStrings.end());
	}
	catch (const XException& e)
	{
		throw XException(e, L"XSoundBank::XSoundBankParser::Parse(): parser error");
	}

}


// <FILE ...список атрибутов... /> 
// Атрибуты:
// name (STRING)         - имя файла, обязательный
// id (INT)              - идентификатор файла, обязательный
// type (STRING)         - тип файла (fetch / stream), обязательный
// description (STRING)  - описание, необязательный

bool XSoundBankParser::FILE_Handler(const XStringParser& obj, const std::wsmatch& match)
{
	XSoundBankParser& p = (XSoundBankParser&)const_cast<XStringParser&>(obj);

	XTagAttribute attrName{ L"NAME", XTagAttributeType::TYPE_STRING, true, std::make_any<XTagAttribute::STRING_TYPE>() };
	XTagAttribute attrId{ L"ID", XTagAttributeType::TYPE_INT, true, std::make_any<XTagAttribute::INT_TYPE>(0) };
	XTagAttribute attrType{ L"TYPE", XTagAttributeType::TYPE_STRING, true, std::make_any<XTagAttribute::STRING_TYPE>() };
	XTagAttribute attrDescription{ L"DESCRIPTION", XTagAttributeType::TYPE_STRING, false, std::make_any<XTagAttribute::STRING_TYPE>(L"") };

	std::wstring strAttributes = match[1];

	try
	{
		ParseAttributes(strAttributes, { &attrName, &attrId, &attrType, &attrDescription });
	}
	catch (XException &e)
	{
		throw XException(e, L"XSoundBankParser::FILE_Handler(): parsing error");
	}

	XTagAttribute::STRING_TYPE strName = std::any_cast<XTagAttribute::STRING_TYPE>(attrName.GetValue());
	XTagAttribute::INT_TYPE    nId     = std::any_cast<XTagAttribute::INT_TYPE>(attrId.GetValue());
	XTagAttribute::STRING_TYPE strType = std::any_cast<XTagAttribute::STRING_TYPE>(attrType.GetValue());
	XTagAttribute::STRING_TYPE strDesc = std::any_cast<XTagAttribute::STRING_TYPE>(attrDescription.GetValue());

	// Проверка на допустимость значений.

	if (nId > 0xFFFF)
	{
		throw XException(L"XSoundBankParser::FILE_Handler(): ID attribute has invalid value");
	}

	for (auto &e : strType)
	{
		e = std::towupper(e);
	}

	if ( (strType != L"FETCH") && (strType != L"STREAM"))
	{
		throw XException(L"XSoundBankParser::FILE_Handler(): TYPE attribute has invalid value");
	}

	if (p.m_usetId.find(static_cast<uint16_t>(nId)) != p.m_usetId.end())
	{
		throw XException(L"XSoundBankParser::FILE_Handler(): ID attribute is already used");
	}
	else
	{
		p.m_usetId.emplace(static_cast<uint16_t>(nId));
	}
	

	// Добавляем новую запись.

	XSoundBankEntry Entry;

	Entry.nId = static_cast<uint16_t>(nId);
	Entry.strFileName = strName;
	Entry.bFetch = (strType == L"FETCH") ? true : false;
	Entry.bStream = (strType == L"STREAM") ? true : false;
	Entry.strDescription = strDesc;

	p.m_pDestList->push_back(std::move(Entry));

	return true;
}

void XSoundBankParser::ParseAttributes(const std::wstring s, std::initializer_list<XTagAttribute*> AttrList)
{
	std::wstring strChunk{};
	wchar_t      cCurrent;
	bool         bProcessing{ false };

	std::vector<std::wstring> vecChunks;

	// Вспомогательная функция -- преобразование строки в верхний регистр.
	auto string_to_upper = [](std::wstring s)
	{
		for (auto &l : s)
		{
			l = std::towupper(l);
		};

		return s;
	};

	// Движемся слева - направо, отбрасывая пробелы.

	for (size_t i = 0; i < s.length(); ++i)
	{
		cCurrent = s[i];

		if (cCurrent == L' ')
		{
			// Достигли пробела. Пробел -- разделитель атрибутов.
			if (!bProcessing)
			{
				// Мы не внутри чанка, просто смещаемся вдоль строки.
				continue;
			}
			else
			{
				// Внутри чанка. Обработка прекращается, добавляем чанк в список. 
				vecChunks.push_back(string_to_upper(strChunk));
				bProcessing = false;

				continue;
			}
		}
		else
		{
			// Какой-то символ. Начинаем/продолжаем обработку.

			// Проверяем, не является ли символ кавычками. Если так, двигаемся до конца, пока не встретим
			// замыкающие кавычки.

			if (cCurrent == L'\"')
			{
				bool bStringComplete{ false };
				strChunk = s[i++];

				while (i < s.length())
				{
					strChunk += s[i];

					if (s[i] == L'\"')
					{
						// Нашли замыкающие кавычки.
						bStringComplete = true;
						break;
					}
				
					++i;
				}

				if (!bStringComplete)
				{
					// Замыкающих кавчыек нет.
					throw XException(L"XSoundBankParser::ParseAttributes(): unexpected end of string");
				}
				else
				{
					// Строка сформирована. После нее обязательно должен строять пробел или конец строки.
					if (i < s.length() - 1)
					{
						if (s[i + 1] != L' ')
						{
							throw XException(L"XSoundBankParser::ParseAttributes(): invalid string specified");
						}
					};
					
					// Заталкиваем строку в вектор.

					vecChunks.push_back(strChunk);

					continue;

				}
			};

			// Проверяем, не является ли символ знаком равенства. В этом случае он сам по себе
			// является разделителем. Добавляем и его тоже.
			if (cCurrent == L'=')
			{
				if (bProcessing)
				{
					// Да, досрочное завершение.
					bProcessing = false;

					vecChunks.push_back(string_to_upper(strChunk));
				}

				vecChunks.push_back(L"=");

				continue;
			}
			else
			{
				if (!bProcessing)
				{
					// Начинаем обработку очередного чанка.
					bProcessing = true;
					strChunk = cCurrent;

					continue;
				}
				else
				{
					// Продолжаем обработку текущего чанка.
					strChunk += cCurrent;

					continue;
				}
			}
		}
	}

	if (bProcessing)
	{
		// Строка кончилась, заталкиваем остаток.
		vecChunks.push_back(string_to_upper(strChunk));
	}

	// Таблица будет содержать классы атрибутов по ключу с именем. Причем имя -- в верхнем регистре.
	std::unordered_map<std::wstring, XTagAttribute*> mapAttr;

	for (auto &e : AttrList)
	{
		mapAttr[string_to_upper(e->GetName())] = e;
	};

	// Теперь проходим по всем найденным чанкам и заполняем атрибуты значениями.

	XTagAttribute* pCurrentAttr{ nullptr };

	for (size_t k = 0; k < vecChunks.size(); ++k)
	{
		if (!pCurrentAttr)
		{
			// Текущего атрибута нет -- ищем заданный и делаем его текущим.
			try
			{
				pCurrentAttr = mapAttr.at(vecChunks[k]);
			}
			catch (std::out_of_range& e)
			{
				UNREFERENCED_PARAMETER(e);

				// Нет такого атрибута. Бросаем исключение.
				throw XException(L"XSoundBankParser::ParseAttributes(): unknown attribute '%s'", vecChunks[k].c_str());
			}
		}
		else
		{
			// Текущий атрибут есть. За ним должен идти знак равенства и далее -- значение.
			if (vecChunks[k++] != L"=")
			{
				// Знака присваивания нет -- ошибка.
				throw XException(L"XSoundBankParser::ParseAttributes(): '=' expected for attribute '%s'", vecChunks[k - 1].c_str());
			}

			if (k > (vecChunks.size() - 1))
			{
				// Значения нет -- ошибка.
				throw XException(L"XSoundBankParser::ParseAttributes(): value expected for attribute '%s'", vecChunks[k - 1].c_str());
			}

			pCurrentAttr->SetValue(vecChunks[k]);
			pCurrentAttr = nullptr;
		}
	}

	if (pCurrentAttr)
	{
		// Атрибут остался незаданным -- ошибка.
		throw XException(L"XSoundBankParser::ParseAttributes(): value expected for attribute '%s'", pCurrentAttr->GetName().c_str());
	}

	// Проверям, чтобы все обязательные атрибуты были заданы.
	for (auto &e : AttrList)
	{
		if ((e->IsRequired()) && (!e->IsDefined()))
		{
			throw XException(L"XSoundBankParser::ParseAttributes(): attribute '%s' is required but not set", e->GetName().c_str());
		}
	}
};
