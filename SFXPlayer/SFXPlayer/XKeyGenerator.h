#pragma once

#include <unordered_set>

class XKeyGenerator
{
private:
	uint32_t                     m_nKeysEmitted;
	uint32_t                     m_nMaxKeys;
	std::unordered_set<uint32_t> m_setKeyPool;

public:
	XKeyGenerator(uint32_t nMaxKeys);

	XKeyGenerator& operator >> (uint32_t& key) ;

	XKeyGenerator& operator << (uint32_t key);
};

