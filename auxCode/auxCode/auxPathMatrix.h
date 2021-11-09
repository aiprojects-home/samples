#pragma once

#include <vector>
#include <list>
#include <iostream>

/*

 ƒанные матрицы выгл€д€т в пам€ти так: (unsigned int - unsigned int - ... unsigned int)
 “о есть выравнивание - по границе 32 бит.

 (0 1 2 ... 31) - (0 1 2 ... 31) - ... - (0 1 2 ... 31)
 (0 1 2 ... 31) - (0 1 2 ... 31) - ... - (0 1 2 ... 31)
 (0 1 2 ... 31) - (0 1 2 ... 31) - ... - (0 1 2 ... 31)
 ...
 (0 1 2 ... 31) - (0 1 2 ... 31) - ... - (0 1 2 ... 31)

*/

namespace aux
{
	class PathMatrix
	{
	public:
		unsigned int m_Width;             // ширина матрицы
		unsigned int m_Height;            // высота матрицы
	private:
		unsigned int m_LineBytes;         // длина строки в байтах (выравнивание по границе 32х бит)

		std::vector<unsigned int> m_Data; // данные

	public:
		PathMatrix();
		PathMatrix(const PathMatrix& obj);

		// ¬озвращает TRUE, если матрица не инициализирована:
		bool IsEmpty();

		// —оздание пустой матрицы по заданным размерам:
		bool Create(const unsigned int w, const unsigned int h);

		// —оздание матрицы по заданным размерам и установка данных из списка. 
		bool Create(const unsigned int w, const unsigned int h, std::initializer_list<char> data);

		// ќчистка матрицы, удаление всех данных:
		bool Destroy();

		// ќчистка матрицы без удалени€ данных:
		bool Clear();

		// ”становка бита в dwBit (младший бит) по заданным координатам:
		bool SetBit(const unsigned int x, const unsigned int y, const unsigned int b);
		bool SetBitFast(const unsigned int x, const unsigned int y, const unsigned int b);

		// ѕолучение бита по заданным координатам:
		bool GetBit(const unsigned int x, const unsigned int y, unsigned int& b);
		bool GetBitFast(const unsigned int x, const unsigned int y, unsigned int& b);

		void Dump(std::ostream &os);

		// ѕоиск пути из одной точки в другую (sx, sy) -> (dx, dy).
		bool FindPath(const std::pair<unsigned int, unsigned int> s, const std::pair<unsigned int, unsigned int> d,
			std::list<std::pair<unsigned int, unsigned int>>& lpath);
	};
}
