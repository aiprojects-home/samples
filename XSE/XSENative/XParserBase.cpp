#include "pch.h"
#include "XParserBase.h"

XParserElement::XParserElement()
{
	m_Type = XParserElementType::EmptyElement;
}

XParserElement::XParserElement(const std::initializer_list<XParserElement>& e)
{
	m_Type = XParserElementType::RootElement;

	m_ChildElements.assign(e);
}

XParserElement::XParserElement(std::wregex begin, std::function<XParserHandler> beginHandler,
	std::wregex end, std::function<XParserHandler> endHandler,
	const std::initializer_list<XParserElement>& innerElements)
{
	m_Type = XParserElementType::GroupElement;

	m_RegexBegin = begin;
	m_BeginHandler = beginHandler;

	m_RegexEnd = end;
	m_EndHandler = endHandler;

	m_ChildElements.assign(innerElements);
}

XParserElement::XParserElement(std::wregex element, std::function<XParserHandler> handler)
{
	m_Type = XParserElementType::SingleElement;

	m_RegexBegin = element;
	m_BeginHandler = handler;
}

XParserElement::XParserElement(const XParserElement& other)
{
	m_Type = other.m_Type;

	m_RegexBegin = other.m_RegexBegin;
	m_RegexEnd = other.m_RegexEnd;

	m_BeginHandler = other.m_BeginHandler;
	m_EndHandler = other.m_EndHandler;

	m_ChildElements = other.m_ChildElements;
}

XParserElement::XParserElement(XParserElement&& other)
{
	m_Type = std::exchange(other.m_Type, XParserElementType::EmptyElement);

	m_RegexBegin = std::move(other.m_RegexBegin);
	m_RegexEnd = std::move(other.m_RegexEnd);

	m_BeginHandler = std::move(other.m_BeginHandler);
	m_EndHandler = std::move(other.m_EndHandler);

	m_ChildElements = std::move(other.m_ChildElements);
}

XParserElement& XParserElement::operator = (const XParserElement& other)
{
	if (this != &other)
	{
		m_Type = other.m_Type;

		m_RegexBegin = other.m_RegexBegin;
		m_RegexEnd = other.m_RegexEnd;

		m_BeginHandler = other.m_BeginHandler;
		m_EndHandler = other.m_EndHandler;

		m_ChildElements = other.m_ChildElements;
	}

	return *this;
}

XParserElement& XParserElement::operator = (XParserElement&& other)
{
	if (this != &other)
	{
		m_Type = std::exchange(other.m_Type, XParserElementType::EmptyElement);

		m_RegexBegin = std::move(other.m_RegexBegin);
		m_RegexEnd = std::move(other.m_RegexEnd);

		m_BeginHandler = std::move(other.m_BeginHandler);
		m_EndHandler = std::move(other.m_EndHandler);

		m_ChildElements = std::move(other.m_ChildElements);
	}

	return *this;
}

XStringParser::XStringParser()
{

}

void XStringParser::Init(const std::initializer_list<XParserElement> &e)
{
	m_RootElement = XParserElement(e);
}

