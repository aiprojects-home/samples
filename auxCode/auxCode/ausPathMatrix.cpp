#include "auxPathMatrix.h"
#include <algorithm>

namespace aux
{
	PathMatrix::PathMatrix()
	{
		m_Width = m_Height = 0;
		m_LineBytes = 0;
	}

	PathMatrix::PathMatrix(const PathMatrix& obj)
	{
		m_Width = obj.m_Width;
		m_Height = obj.m_Height;
		m_LineBytes = obj.m_LineBytes;

		m_Data = obj.m_Data;
	}

	bool PathMatrix::IsEmpty()
	{
		return !((m_Width) && (m_Height));
	}

	bool PathMatrix::Create(const unsigned int w, const unsigned int h)
	{

		if (!IsEmpty())
		{
			// ������� ��� �������:
			return false;
		};

		unsigned int alignedw;   // ������ � ������ ������������
		unsigned int missedbits; // ����������� ����

		// ������� ��������� �������� ������ (��� ������ �������� �� 32, ��������� ����������� �� 32):
		if (missedbits = (w % 32))
		{
			// ���� ������� - ���������� ���:
			alignedw = w + (32 - missedbits);
		}
		else
		{
			// ������� ��� - ������ ��� ��������� �� 32:
			alignedw = w;
		};

		// ���������� ������ � ������ ����� (�������� ��������, ��� ������������):
		m_Width = w;
		m_Height = h;

		// ��������� ���������� ���� ��� �������� ����� ������ � ���� ������� (��� � �������������):
		m_LineBytes = (alignedw / 8);
		m_Data.resize(m_LineBytes * h, 0);

		return true;
	}

	bool PathMatrix::Create(const unsigned int w, const unsigned int h, std::initializer_list<char> data)
	{
		if (data.size() < w * h)
			return false;

		if (!Create(w, h))
			return false;

		// ������ �������� �����-�������, ������-����.
		unsigned int cur_x{ 0 }, cur_y{ 0 };
		for (auto &e : data)
		{
			SetBit(cur_x, cur_y, (e ? 1 : 0));

			if (++cur_x >= w)
			{
				cur_x = 0;
				cur_y++;
			};
		}

		return true;
	}


	bool PathMatrix::Destroy()
	{
		if (!IsEmpty())
		{
			// ������� ������ ���� ���� ������:
			m_Width = m_Height = 0;
			m_LineBytes = 0;

			m_Data.clear();
			m_Data.shrink_to_fit();

			return true;
		};

		return false;
	};

	bool PathMatrix::Clear()
	{
		if (!IsEmpty())
		{
			// ������� ������ ������ ���� ��� ����:
			std::fill(m_Data.begin(), m_Data.end(), 0);

			return true;
		};

		// ������ ��� - ����� � �������:

		return false;
	};

	bool PathMatrix::SetBit(const unsigned int x, const unsigned int y, const unsigned int b)
	{
		if ((x >= m_Width) || (y >= m_Height) || (IsEmpty()))
			return false;

		return SetBitFast(x, y, b);
	}

	bool PathMatrix::SetBitFast(const unsigned int x, const unsigned int y, const unsigned int b)
	{
		// ��������� ������ ������� ���.
		unsigned int bit{ b & 1 };

		// ��������� �������� (������):
		size_t offset{ (((y * m_LineBytes) / 4) + (x / 32)) };

		unsigned int d = m_Data[offset];

		// ������� ������� ���, ������� ���� ����������:
		d &= ~(((unsigned int)1 << (x % 32)));

		// ������������� ���:
		d |= ((unsigned int)bit << (x % 32));

		m_Data[offset] = d;

		return true;
	}

	bool PathMatrix::GetBit(const unsigned int x, const unsigned int y, unsigned int& b)
	{
		if ((x >= m_Width) || (y >= m_Height) || (IsEmpty()))
			return false;

		return GetBitFast(x, y, b);
	}

	bool PathMatrix::GetBitFast(const unsigned int x, const unsigned int y, unsigned int& b)
	{
		// ��������� �������� (������):
		size_t offset{ (((y * m_LineBytes) / 4) + (x / 32)) };

		// �������� ���:
		b = m_Data[offset];
		b &= ((unsigned int)1 << (x % 32));
		b >>= (x % 32);

		return true;
	}

	void PathMatrix::Dump(std::ostream &os)
	{
		if (IsEmpty())
			return;

		unsigned int bit;

		for (unsigned int y = 0; y < m_Height; y++)
		{
			for (unsigned int x = 0; x < m_Width; x++)
			{
				GetBitFast(x, y, bit);

				os << (bit ? '1' : '0');
			}

			os << std::endl;
		}
	}

	bool PathMatrix::FindPath(const std::pair<unsigned int, unsigned int> s, const std::pair<unsigned int, unsigned int> d,
		std::list<std::pair<unsigned int, unsigned int>>& lpath)
	{
		using step_type = std::pair<unsigned int, unsigned int>;
		using path_type = std::list<step_type>;

		// ������ ��������� ������� �� ������� path_type.
		// ���� �� ����� ����� � ������� ���� ��������� (����� ��� =1), �� ����� ���� �� ������������
		// std::priority_queue � ����������� �� ����� ����.

		std::list<path_type> vl;

		// ����� ������� ��� ������� ����.
		PathMatrix mtx(*this);

		// ������� ���� ��� ��������� (�������� �� ��������� �����).
		path_type cur_path{ s };
		step_type last_step, next_step;

		// �������� � ������ �����.
		vl.push_back(cur_path);

		auto CheckStep = [&mtx, &cur_path, &vl, &last_step, &next_step](unsigned int x, unsigned int y)
		{
			if (unsigned int bit{ 1 }; mtx.GetBit(x, y, bit))
			{
				if (!bit)
				{
					// ������ ��������.
					next_step = std::make_pair(x, y);

					// ��������� ��������� ������� ����.
					mtx.SetBitFast(next_step.first, next_step.second, 1);

					cur_path.push_back(next_step);
					vl.push_back(cur_path);

					// ���������� ������� ���� � �������� ��������� (��� ������ �����������).
					cur_path.pop_back();
				}
			}
		};

		do
		{
			// �������� �� ������� ������� ����.
			cur_path = vl.front();
			vl.pop_front();

			// ���������, �������� �� �� ������:
			last_step = cur_path.back();
			if (last_step == d)
			{
				// ��, ��������� ��� ��������� � �������.
				lpath = cur_path;

				return true;
			};

			// ���, ����� ������ ������ (�� ���� ��������).

			CheckStep(last_step.first - 1, last_step.second);
			CheckStep(last_step.first + 1, last_step.second);
			CheckStep(last_step.first, last_step.second - 1);
			CheckStep(last_step.first, last_step.second + 1);

		} while (vl.size()); // ���� �� ����������� ��������

		return false;
	}

	void PathDemo()
	{
		PathMatrix pm;

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
			// ���������� ������� � ����.
			unsigned int bit;
			decltype(path)::const_iterator path_iter;

			for (unsigned int y = 0; y < pm.m_Height; y++)
			{
				for (unsigned int x = 0; x < pm.m_Width; x++)
				{
					pm.GetBitFast(x, y, bit);

					// ���� �� ������ ����������� - ����� ����, ������� '*':
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
	}
}