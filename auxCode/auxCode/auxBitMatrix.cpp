#include "auxBitMatrix.h"
#include "windows.h"

auxBitMatrix::auxBitMatrix(uint32_t nWidth, uint32_t nHeight, uint8_t nKeySize)
{
	m_nMtxWidth = nWidth;
	m_nMtxHeight = nHeight;
	m_nBitKeySize = nKeySize;

	m_nByteWidth = m_nMtxWidth * m_nBitKeySize / 8;
	if ((m_nMtxWidth * m_nBitKeySize) % 8)
	{
		m_nByteWidth++;
	}

	m_vecData.resize(m_nByteWidth * m_nMtxHeight);
}

void auxBitMatrix::SetValue(uint32_t x, uint32_t y, uint32_t value)
{
	uint32_t nCurIndex{ 0 };
	uint8_t  nCurOffset{ 0 };
	uint32_t nCurValue{ value };

	MapToIndex(x, y, nCurIndex, nCurOffset);

	uint8_t nBitsLeft{ m_nBitKeySize };
	uint8_t nChunkSize{ 0 };
	uint8_t nValueChunk{ 0 };
	uint8_t nCombValue{ 0 };

	do
	{
		nChunkSize = min(nBitsLeft, 8 - nCurOffset);
		nValueChunk = static_cast<uint8_t>( nCurValue & ( (1 << nChunkSize) - 1 ) );
		nCurValue = (nCurValue >> nChunkSize);
		nBitsLeft -= nChunkSize;

		nCombValue = m_vecData[nCurIndex];
		nCombValue &= ~ (((1 << nChunkSize) - 1) << (8 - nChunkSize - nCurOffset));
		nCombValue |= (nValueChunk << (8 - nChunkSize - nCurOffset));
		m_vecData[nCurIndex] = nCombValue;

		nCurIndex++;
		nCurOffset = 0;

	} while (nBitsLeft);
}

uint32_t auxBitMatrix::GetValue(uint32_t x, uint32_t y)
{
	uint32_t nCurIndex{ 0 };
	uint8_t  nCurOffset{ 0 };
	uint32_t nCurValue{ 0 };

	MapToIndex(x, y, nCurIndex, nCurOffset);

	uint8_t  nBitsLeft{ m_nBitKeySize };
	uint8_t  nChunkSize{ 0 };
	uint8_t  nValueChunk{ 0 };
	uint32_t nKeyValue{ 0 };

	do
	{
		nValueChunk = m_vecData[nCurIndex];
		nChunkSize = min(nBitsLeft, 8 - nCurOffset);

		nValueChunk >>= (8 - nChunkSize - nCurOffset);
		nValueChunk &= ( (1 << nChunkSize) - 1);

		nKeyValue |= (nValueChunk << (m_nBitKeySize - nBitsLeft));

		nBitsLeft -= nChunkSize;

		nCurIndex++;
		nCurOffset = 0;

	} while (nBitsLeft);

	return nKeyValue;
}

void auxBitMatrix::MapToIndex(uint32_t x, uint32_t y, uint32_t& index, uint8_t& offset)
{
	index = (y * m_nByteWidth) + (x * m_nBitKeySize / 8);
	offset = (x * m_nBitKeySize) % 8;
}
