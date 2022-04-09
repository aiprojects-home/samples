#pragma once

#include "XGlobals.h"
#include "XParserBase.h"

class XSoundBank;
class XSoundDecoder;

enum class XTagAttributeType
{
	TYPE_STRING,
	TYPE_INT,
	TYPE_BOOL,
	TYPE_FLOAT
};

// Класс для обработки строковых артибутов внутри тега.

class XTagAttribute
{
public:

	using STRING_TYPE = std::wstring;  // Типы C++ соответствующие типам атрибутов
	using INT_TYPE    = unsigned long;
	using BOOL_TYPE   = bool;
	using FLOAT_TYPE  = float;

private:

	std::wstring      m_strAttrName;   // имя атрибута
	XTagAttributeType m_Type;          // тип
	std::any          m_anyValue;      // значение
	bool              m_bRequired;     // флаг, является ли обязательным 
	bool              m_bDefined;      // флаг, что атрибут задан

public:

	XTagAttribute(std::wstring strName, XTagAttributeType Type, bool bRequired, std::any anyDefaultValue);

	void SetValue(std::wstring strValue);
	
	bool IsDefined();
	
	bool IsRequired();
	
	std::wstring GetName();
	
	std::any GetValue();
};

class XSoundBankEntry
{
public:

	uint16_t                       nId;
	std::wstring                   strFileName;
	std::shared_ptr<XSoundDecoder> spDecoder;
	bool                           bFetch;
	bool                           bStream;
	std::wstring                   strDescription;

	XSoundBankEntry();
	XSoundBankEntry(XSoundBankEntry& other) = delete;
	XSoundBankEntry(XSoundBankEntry&& other);

	XSoundBankEntry& operator = (XSoundBankEntry&& other);
};

class XSoundBankParser : XStringParser
{
private:

	std::list<XSoundBankEntry>   *m_pDestList;
	std::unordered_set<uint16_t>  m_usetId;

public:

	XSoundBankParser();
	void Parse(const wchar_t* pFileName, std::list<XSoundBankEntry>* pDest);


private:
	
	static bool FILE_Handler(const XStringParser& obj, const std::wsmatch& match);
	static void ParseAttributes(const std::wstring s, std::initializer_list<XTagAttribute*> AttrList);

};

