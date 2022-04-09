#pragma once

#include "XGlobals.h"
#include "XException.h"

// Element types.
enum XParserElementType : int
{
	EmptyElement = 0,   // затычка
	RootElement = 1,    // корневой элемент, может содержать группу или одиночный элемент
	SingleElement = 2,  // одиночный элемент, ничего не содержит
	GroupElement = 3    // групповой элемент, может содержать как одиночные так и групповые элементы
};

class XStringParser;

using XParserHandler = bool(const XStringParser&, const std::wsmatch&);

struct XParserElement
{
	friend class XStringParser;

private:
	XParserElementType            m_Type;          // тип элемента
	std::wregex                   m_RegexBegin;    // регулярное выражение для входа в обработку
	std::wregex                   m_RegexEnd;      // регулярное выражение для выхода

	std::function<XParserHandler> m_BeginHandler;  // обработчик при входе
	std::function<XParserHandler> m_EndHandler;    // обработчик при выходе

	std::vector<XParserElement>   m_ChildElements; // внутренние элементы (если есть)

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
	XParserElement m_RootElement; // начало логического дерева элементов

public:
	XStringParser();

	// Метод инициализирует парсер корневым элементом.
	void Init(const std::initializer_list<XParserElement>& e);

	// Метод начинает парсинг контейнера по заданным итераторам.
	template <typename IT>
	void Parse(IT itbegin, IT itend) const
	{
		std::stack<const XParserElement*> es; // стек, чтобы определять текущее метоположение в дереве
		uint32_t line{ 0 };;                  // номер строки для отчета об ошибках


		es.push(&m_RootElement);

		std::wsmatch base_match;
		bool processed;
		typename IT::value_type s;

		for (IT it = itbegin; it != itend; ++it)
		{
			s = *it;
			line = (uint32_t)(std::distance(itbegin, it)) + 1;
			processed = false;

			// Пропускаем пустые строки и строки из пробелов.
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
						// Обработчик не был задан, но элемент все равно должен быть обработан.
						UNREFERENCED_PARAMETER(e);
					}
					catch (const XException& e)
					{
						// Обработчик вызвал исключение:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					if (e.m_Type == XParserElementType::GroupElement)
					{
						// Входим в группу.
						es.push(&e);
					}

					processed = true;

					break;
				}
			}

			if ((es.top()->m_Type == XParserElementType::GroupElement) && (!processed))
			{
				// Может быть пора покидать текущую секцию?
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
						// Обработчик вызвал исключение:
						throw XException(e, L"XStringParser::Parse(): can't parse line %d", line);
					}

					// Поднимаемся на предыдущий уровень.
					es.pop();
				}
			}

			if (!processed)
			{
				// Такого обработчика нет -- вызываем исключение:

				throw XException(L"XStringParser::Parse(): can't parse line %d", line);
			}
		}
	}
};
