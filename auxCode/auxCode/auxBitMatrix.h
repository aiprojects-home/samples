#pragma once

#include <vector>

class auxBitMatrix
{
private:
	uint32_t m_nMtxWidth;
	uint32_t m_nMtxHeight;
	uint8_t  m_nBitKeySize;

	uint32_t m_nByteWidth;

	std::vector<uint8_t> m_vecData;

public:

	auxBitMatrix(uint32_t nWidth, uint32_t nHeight, uint8_t nKeySize);

	void SetValue(uint32_t x, uint32_t y, uint32_t value);

	uint32_t GetValue(uint32_t x, uint32_t y);

private:

	void MapToIndex(uint32_t x, uint32_t y, uint32_t& index, uint8_t& offset);
};

