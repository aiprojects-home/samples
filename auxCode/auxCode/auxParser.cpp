#include "auxParser.h"

namespace aux
{
	ParserElement::ParserElement()
	{
		m_Type = ParserElementType::EmptyElement;
	}

	ParserElement::ParserElement(const std::initializer_list<ParserElement>& e)
	{
		m_Type = ParserElementType::RootElement;

		m_ChildElements.assign(e);
	}

	ParserElement::ParserElement(std::regex begin, std::function<ParserHandler> beginHandler,
		std::regex end, std::function<ParserHandler> endHandler,
		const std::initializer_list<ParserElement>& innerElements)
	{
		m_Type = ParserElementType::GroupElement;

		m_RegexBegin = begin;
		m_BeginHandler = beginHandler;

		m_RegexEnd = end;
		m_EndHandler = endHandler;

		m_ChildElements.assign(innerElements);
	}

	ParserElement::ParserElement(std::regex element, std::function<ParserHandler> handler)
	{
		m_Type = ParserElementType::SingleElement;

		m_RegexBegin = element;
		m_BeginHandler = handler;
	}

	ParserElement::ParserElement(const ParserElement& other)
	{
		m_Type = other.m_Type;
		
		m_RegexBegin = other.m_RegexBegin;
		m_RegexEnd = other.m_RegexEnd;

		m_BeginHandler = other.m_BeginHandler;
		m_EndHandler = other.m_EndHandler;

		m_ChildElements = other.m_ChildElements;
	}

	ParserElement::ParserElement(ParserElement&& other)
	{
		m_Type = std::exchange(other.m_Type, ParserElementType::EmptyElement);

		m_RegexBegin = std::move(other.m_RegexBegin);
		m_RegexEnd = std::move(other.m_RegexEnd);

		m_BeginHandler = std::move(other.m_BeginHandler);
		m_EndHandler = std::move(other.m_EndHandler);

		m_ChildElements = std::move(other.m_ChildElements);
	}

	ParserElement& ParserElement::operator = (const ParserElement& other)
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

	ParserElement& ParserElement::operator = (ParserElement&& other)
	{
		if (this != &other)
		{
			m_Type = std::exchange(other.m_Type, ParserElementType::EmptyElement);

			m_RegexBegin = std::move(other.m_RegexBegin);
			m_RegexEnd = std::move(other.m_RegexEnd);

			m_BeginHandler = std::move(other.m_BeginHandler);
			m_EndHandler = std::move(other.m_EndHandler);

			m_ChildElements = std::move(other.m_ChildElements);
		}

		return *this;
	}

	StringParser::StringParser()
	{

	}

	void StringParser::Init(const std::initializer_list<ParserElement> &e)
	{
		m_RootElement = ParserElement(e);
	}

}