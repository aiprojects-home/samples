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
			// ћатрица уже создана:
			return false;
		};

		unsigned int alignedw;   // ширина с учетом выравнивани€
		unsigned int missedbits; // недостающие биты

		// —начала провер€ем заданную ширину (она должна делитьс€ на 32, поскольку выравниваем по 32):
		if (missedbits = (w % 32))
		{
			// ≈сть остаток - прибавл€ем его:
			alignedw = w + (32 - missedbits);
		}
		else
		{
			// ќстатка нет - ширина уже выровнена по 32:
			alignedw = w;
		};

		// «апоминаем ширину и высоту маски (реальные значени€, без выравнивани€):
		m_Width = w;
		m_Height = h;

		// ¬ычисл€ем количество байт дл€ хранени€ одной строки и всей матрицы (уже с выравниванием):
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

		// ƒанные записаны слева-направо, сверху-вниз.
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
			// ”дал€ем только если есть данные:
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
			// ќчищаем данные только если они есть:
			std::fill(m_Data.begin(), m_Data.end(), 0);

			return true;
		};

		// ƒанных нет - выход с ошибкой:

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
		// ќставл€ем только младший бит.
		unsigned int bit{ b & 1 };

		// ¬ычисл€ем смещение (индекс):
		size_t offset{ (((y * m_LineBytes) / 4) + (x / 32)) };

		unsigned int d = m_Data[offset];

		// —начала очищаем бит, который надо установить:
		d &= ~(((unsigned int)1 << (x % 32)));

		// ”станавливаем бит:
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
		// ¬ычисл€ем смещение (индекс):
		size_t offset{ (((y * m_LineBytes) / 4) + (x / 32)) };

		// ѕолучаем бит:
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

		// —писок вариантов состоит из списков path_type.
		// ≈сли бы длина шагов в матрице была различной (здесь она =1), то нужно было бы использовать
		// std::priority_queue с сортировкой по длине пути.

		std::list<path_type> vl;

		//  опи€ матрицы дл€ отметок пути.
		PathMatrix mtx(*this);

		// “екущий путь дл€ обработки (начинаем со стартовой точки).
		path_type cur_path{ s };
		step_type last_step, next_step;

		// Ќачинаем с первой точки.
		vl.push_back(cur_path);

		auto CheckStep = [&mtx, &cur_path, &vl, &last_step, &next_step](unsigned int x, unsigned int y)
		{
			if (unsigned int bit{ 1 }; mtx.GetBit(x, y, bit))
			{
				if (!bit)
				{
					// ѕроход свободен.
					next_step = std::make_pair(x, y);

					// ƒобавл€ем очередной вариант пути.
					mtx.SetBitFast(next_step.first, next_step.second, 1);

					cur_path.push_back(next_step);
					vl.push_back(cur_path);

					// ¬озвращаем текущий путь в исходное состо€ние (дл€ других направлений).
					cur_path.pop_back();
				}
			}
		};

		do
		{
			// ¬ыбираем из очереди вариант пути.
			cur_path = vl.front();
			vl.pop_front();

			// ѕровер€ем, достигли ли мы финиша:
			last_step = cur_path.back();
			if (last_step == d)
			{
				// ƒа, последний шаг совпадает с целевым.
				lpath = cur_path;

				return true;
			};

			// Ќет, нужно искать дальше (по всем сторонам).

			CheckStep(last_step.first - 1, last_step.second);
			CheckStep(last_step.first + 1, last_step.second);
			CheckStep(last_step.first, last_step.second - 1);
			CheckStep(last_step.first, last_step.second + 1);

		} while (vl.size()); // пока не закончились варианты

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
	}
}