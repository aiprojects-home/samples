#pragma once

#include <vector>
#include <list>
#include <iostream>

/*

 ������ ������� �������� � ������ ���: (unsigned int - unsigned int - ... unsigned int)
 �� ���� ������������ - �� ������� 32 ���.

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
		unsigned int m_Width;             // ������ �������
		unsigned int m_Height;            // ������ �������
	private:
		unsigned int m_LineBytes;         // ����� ������ � ������ (������������ �� ������� 32� ���)

		std::vector<unsigned int> m_Data; // ������

	public:
		PathMatrix();
		PathMatrix(const PathMatrix& obj);

		// ���������� TRUE, ���� ������� �� ����������������:
		bool IsEmpty();

		// �������� ������ ������� �� �������� ��������:
		bool Create(const unsigned int w, const unsigned int h);

		// �������� ������� �� �������� �������� � ��������� ������ �� ������. 
		bool Create(const unsigned int w, const unsigned int h, std::initializer_list<char> data);

		// ������� �������, �������� ���� ������:
		bool Destroy();

		// ������� ������� ��� �������� ������:
		bool Clear();

		// ��������� ���� � dwBit (������� ���) �� �������� �����������:
		bool SetBit(const unsigned int x, const unsigned int y, const unsigned int b);
		bool SetBitFast(const unsigned int x, const unsigned int y, const unsigned int b);

		// ��������� ���� �� �������� �����������:
		bool GetBit(const unsigned int x, const unsigned int y, unsigned int& b);
		bool GetBitFast(const unsigned int x, const unsigned int y, unsigned int& b);

		void Dump(std::ostream &os);

		// ����� ���� �� ����� ����� � ������ (sx, sy) -> (dx, dy).
		bool FindPath(const std::pair<unsigned int, unsigned int> s, const std::pair<unsigned int, unsigned int> d,
			std::list<std::pair<unsigned int, unsigned int>>& lpath);
	};
}
