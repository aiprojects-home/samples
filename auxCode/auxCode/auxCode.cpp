// auxCode.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <algorithm>
#include <numeric>
#include "auxLogger.h"
#include "auxParser.h"
#include "auxPathMatrix.h"

class CustomParser : public aux::StringParser
{
private:
	int m_Private;

public:
	CustomParser() : StringParser()
	{
		m_Private = 100500;
	}

	/*
	<sphere>
	    <radius = NNN>
	</sphere>
	<box>
	    <size = NNN>
	</box>
	*/
	void Parse()
	{
		Init({
			aux::ParserElement(std::regex("<sphere>"), nullptr, std::regex("</sphere>"), nullptr,
				{
					aux::ParserElement(std::regex("<radius = ([\\d]+)>"), RadiusHandler)
				}),
			
			aux::ParserElement(std::regex("<box>"), nullptr, std::regex("</box>"), nullptr,
				{
					aux::ParserElement(std::regex("<size = ([\\d]+)>"), SizeHandler)
				})
			});

		std::vector<std::string> file{
		"<sphere>", 
		"<radius = 10>",
		"</sphere>",
		"<box>",
		"<size = 45>",
		"</box>"
		};

		StringParser::Parse(file.cbegin(), file.cend());

	}

	static bool RadiusHandler(const StringParser& obj, const std::smatch& match)
	{
		std::cout << "Radius: " << match[1].str() << "\n";

		return true;
	}

	static bool SizeHandler(const StringParser& obj, const std::smatch& match)
	{
		std::cout << "Size: " << match[1].str() << "\n";

		return true;
	}
};

int main()
{
	/*
	std::ofstream log("e:\\log.txt", std::ios::app);

	aux::Logger::Get().SetLogStream(log);

	aux::Logger::Get() << aux::Logger::Get().Now() << ": Testing logger\n";

	*/

	/*
	aux::PathMatrix pm;

	pm.Create(10, 10, {
		0, 1, 0, 0, 0, 0, 0, 0, 0, 1,
		0, 1, 0, 1, 1, 0, 0, 0, 1, 1,
		0, 1, 0, 0, 0, 1, 1, 0, 0, 1,
		0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
		0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 1, 0, 0, 1, 1, 1, 1, 1, 0,
		0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
		0, 1, 1, 1, 1, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		});

	pm.Dump(std::cout);

	std::cout << std::endl;

	std::list<std::pair<unsigned int, unsigned int>> path;

	bool foundflag = pm.FindPath(std::make_pair(0, 9), std::make_pair(2, 7), path);

	if (foundflag)
	{
		// ѕоказываем матрицу и путь.
		unsigned int bit;
		decltype(path)::const_iterator path_iter;

		for (unsigned int y = 0; y < pm.m_Height; y++)
		{
			for (unsigned int x = 0; x < pm.m_Width; x++)
			{
				pm.GetBitFast(x, y, bit);

				// ≈сли по данным координатам - часть пути, выводим '*':
				path_iter = std::find(path.cbegin(), path.cend(), std::make_pair(x, y));

				if (path_iter != path.cend())
				{
					std::cout << '*';
				}
				else
				{
					std::cout << (bit ? '1' : '0');
				}
			}

			std::cout << std::endl;
		}
	}
	else
	{
		std::cout << "No path." << std::endl;
	}
	*/
	CustomParser().Parse();

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
