#pragma once

#include <iostream>
#include <regex>
#include <functional>
#include <stack>

#include "XException.h"

// Element types.
enum XParserElementType : int
{
	EmptyElement = 0,   // stub
	RootElement = 1,    // root element, can contain groups & single elements
	SingleElement = 2,  // single element, can contain nothing
	GroupElement = 3    // group element, can contain single elements & other groups
};

class XStringParser;

using XParserHandler = bool(const XStringParser&, const std::wsmatch&);

struct XParserElement
{
	friend class XStringParser;

private:
	XParserElementType m_Type;       // type of element
	std::wregex        m_RegexBegin; // regular exp. to enter 
	std::wregex        m_RegexEnd;   // resular exp. to exit

	std::function<XParserHandler> m_BeginHandler;  // function to call when enter
	std::function<XParserHandler> m_EndHandler;    // function to call when exit

	std::vector<XParserElement>   m_ChildElements; // inner elements (for group)

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
	XParserElement m_RootElement; // begin of logical tree

public:
	XStringParser();

	// Method initializes parser with root-type element.
	void Init(const std::initializer_list<XParserElement>& e);

	// Method parses the container with given iterators.
	template <typename IT>
	void Parse(IT itbegin, IT itend) const
	{
		std::stack<const XParserElement*> es; // stack to determine current level in logical tree
		uint32_t line{ 0 };;                  // line number for error report


		es.push(&m_RootElement);

		std::wsmatch base_match;
		bool processed;
		typename IT::value_type s;

		for (IT it = itbegin; it != itend; ++it)
		{
			s = *it;
			line = (uint32_t)(std::distance(itbegin, it)) + 1;
			processed = false;

			// ѕропускаем пустые строки и строки из пробелов.
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
						// Handler was not set, but element has to be processed.
						UNREFERENCED_PARAMETER(e);
					}
					catch (const XException& e)
					{
						// Handler threw an exception:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					if (e.m_Type == XParserElementType::GroupElement)
					{
						// Entering the group.
						es.push(&e);
					}

					processed = true;

					break;
				}
			}

			if ((es.top()->m_Type == XParserElementType::GroupElement) && (!processed))
			{
				// May be we should leave current section?
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
						// Handler threw an exception:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					// Move to upper level in tree.
					es.pop();
				}
			}

			if (!processed)
			{
				// Can't process the line:

				throw XException(L"XStringParser::Parse(): can't parse line %d", line);
			}
		}
	}
};
