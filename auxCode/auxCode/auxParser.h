#pragma once

#include <iostream>
#include <regex>
#include <functional>
#include <stack>

namespace aux
{
	enum ParserElementType : int
	{
		 EmptyElement = 0,
		 RootElement = 1,
		 SingleElement = 2,
		 GroupElement = 3
	};

	class StringParser;

	using ParserHandler = bool (const StringParser&, const std::smatch&);

	struct ParserElement
	{
		friend class StringParser;

	private:
		ParserElementType m_Type;
		std::regex        m_RegexBegin;
		std::regex        m_RegexEnd;
		
		std::function<ParserHandler> m_BeginHandler;
		std::function<ParserHandler> m_EndHandler;

		std::vector<ParserElement>   m_ChildElements;

	public:
		ParserElement();
		
		ParserElement(const std::initializer_list<ParserElement>& e);

		ParserElement(std::regex begin, std::function<ParserHandler> beginHandler,
			std::regex end, std::function<ParserHandler> endHandler,
			const std::initializer_list<ParserElement>& innerElements);

		ParserElement(std::regex element, std::function<ParserHandler> handler);

		ParserElement(const ParserElement& other);

		ParserElement(ParserElement&& other);

		ParserElement& operator = (const ParserElement& other);

		ParserElement& operator = (ParserElement&& other);

	};

	class StringParser
	{
	private:
		ParserElement  m_RootElement;
	
	public:
		StringParser();

		void Init(const std::initializer_list<ParserElement>& e);

		template <typename IT>
		void Parse(IT itbegin, IT itend)
		{
			std::stack<ParserElement*> es;

			es.push(&m_RootElement);

			std::smatch base_match;
			bool processed;
			typename IT::value_type s;

			for (IT it = itbegin; it < itend; ++it)
			{
				s = *it;
				processed = false;

				for(auto &e : es.top()->m_ChildElements)
				{
					if (std::regex_match(s, base_match, e.m_RegexBegin))
					{
						try
						{
							e.m_BeginHandler(*this, base_match);
						} 
						catch (std::bad_function_call e)
						{
						}

						if (e.m_Type == ParserElementType::GroupElement)
						{
							es.push(&e);
						}

						processed = true;

						break;
					}
				}

				if ( (es.top()->m_Type == ParserElementType::GroupElement) && (!processed))
				{
					if (std::regex_match(s, base_match, es.top()->m_RegexEnd))
					{
						try
						{
							es.top()->m_EndHandler(*this, base_match);
						}
						catch (std::bad_function_call e)
						{
						}

						es.pop();
					}
				}
			}
		}
	};
}

