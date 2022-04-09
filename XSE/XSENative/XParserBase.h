#pragma once

#include "XGlobals.h"
#include "XException.h"

// Element types.
enum XParserElementType : int
{
	EmptyElement = 0,   // �������
	RootElement = 1,    // �������� �������, ����� ��������� ������ ��� ��������� �������
	SingleElement = 2,  // ��������� �������, ������ �� ��������
	GroupElement = 3    // ��������� �������, ����� ��������� ��� ��������� ��� � ��������� ��������
};

class XStringParser;

using XParserHandler = bool(const XStringParser&, const std::wsmatch&);

struct XParserElement
{
	friend class XStringParser;

private:
	XParserElementType            m_Type;          // ��� ��������
	std::wregex                   m_RegexBegin;    // ���������� ��������� ��� ����� � ���������
	std::wregex                   m_RegexEnd;      // ���������� ��������� ��� ������

	std::function<XParserHandler> m_BeginHandler;  // ���������� ��� �����
	std::function<XParserHandler> m_EndHandler;    // ���������� ��� ������

	std::vector<XParserElement>   m_ChildElements; // ���������� �������� (���� ����)

public:
	XParserElement();

	XParserElement(const std::initializer_list<XParserElement>& e);

	XParserElement(std::wregex begin, std::function<XParserHandler> beginHandler,
		std::wregex end, std::function<XParserHandler> endHandler,
		const std::initializer_list<XParserElement>& innerElements);

	XParserElement(std::wregex element, std::function<XParserHandler> handler);

	XParserElement(const XParserElement& other);

	XParserElement(XParserElement&& other);

	XParserElement& operator = (const XParserElement& other);

	XParserElement& operator = (XParserElement&& other);

};

class XStringParser
{
private:
	XParserElement m_RootElement; // ������ ����������� ������ ���������

public:
	XStringParser();

	// ����� �������������� ������ �������� ���������.
	void Init(const std::initializer_list<XParserElement>& e);

	// ����� �������� ������� ���������� �� �������� ����������.
	template <typename IT>
	void Parse(IT itbegin, IT itend) const
	{
		std::stack<const XParserElement*> es; // ����, ����� ���������� ������� ������������� � ������
		uint32_t line{ 0 };;                  // ����� ������ ��� ������ �� �������


		es.push(&m_RootElement);

		std::wsmatch base_match;
		bool processed;
		typename IT::value_type s;

		for (IT it = itbegin; it != itend; ++it)
		{
			s = *it;
			line = (uint32_t)(std::distance(itbegin, it)) + 1;
			processed = false;

			// ���������� ������ ������ � ������ �� ��������.
			if ( (std::regex_match(s, base_match, std::wregex(L"(\\s)*"))) || (s == L"") )
				continue;

			for (auto &e : es.top()->m_ChildElements)
			{
				if (std::regex_match(s, base_match, e.m_RegexBegin))
				{
					try
					{
						e.m_BeginHandler(*this, base_match);
					}
					catch (const std::bad_function_call& e)
					{
						// ���������� �� ��� �����, �� ������� ��� ����� ������ ���� ���������.
						UNREFERENCED_PARAMETER(e);
					}
					catch (const XException& e)
					{
						// ���������� ������ ����������:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					if (e.m_Type == XParserElementType::GroupElement)
					{
						// ������ � ������.
						es.push(&e);
					}

					processed = true;

					break;
				}
			}

			if ((es.top()->m_Type == XParserElementType::GroupElement) && (!processed))
			{
				// ����� ���� ���� �������� ������� ������?
				if (std::regex_match(s, base_match, es.top()->m_RegexEnd))
				{
					try
					{
						processed = true;
						es.top()->m_EndHandler(*this, base_match);
					}
					catch (const std::bad_function_call& e)
					{
						UNREFERENCED_PARAMETER(e);
					}
					catch (const XException& e)
					{
						// ���������� ������ ����������:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					// ����������� �� ���������� �������.
					es.pop();
				}
			}

			if (!processed)
			{
				// ������ ����������� ��� -- �������� ����������:

				throw XException(L"XStringParser::Parse(): can't parse line %d", line);
			}
		}
	}
};
