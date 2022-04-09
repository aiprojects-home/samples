#pragma once

#include <unordered_set>

class auxKeyGenerator
{
private:
	uint32_t                     m_nKeysEmitted;
	uint32_t                     m_nMaxKeys;
	std::unordered_set<uint32_t> m_setKeyPool;

public:
	auxKeyGenerator(uint32_t nMaxKeys);

	auxKeyGenerator& operator >> (uint32_t& key) ;

	auxKeyGenerator& operator << (uint32_t key);
};

