#include "auxKeyGenerator.h"

auxKeyGenerator::auxKeyGenerator(uint32_t nMaxKeys)
{
	m_nMaxKeys = nMaxKeys;
	m_nKeysEmitted = 0;
}

auxKeyGenerator& auxKeyGenerator::operator >> (uint32_t& key)
{
	// Check: do we have anythig in our key pool?

	if (m_setKeyPool.size())
	{
		// Return value from the pool.

		auto itFirst = m_setKeyPool.begin();

		key = *itFirst;

		m_setKeyPool.erase(itFirst);
	}
	else
	{
		// Return new value (if we can).

		if (m_nKeysEmitted >= m_nMaxKeys)
		{
			//throw XException(L"XKeyGenerator::operator >>(): No more keys. Max: '%d'.", m_nMaxKeys);
		}
		else
		{
			key = ++m_nKeysEmitted;
		}
	}

	return *this;
}

auxKeyGenerator& auxKeyGenerator::operator << (uint32_t key)
{
	// Key must be lower than m_nKeysEmitted and be missing in the pool.

	if ((key > m_nKeysEmitted) || (m_setKeyPool.find(key) != m_setKeyPool.end()))
	{
		//throw XException(L"XKeyGenerator::operator <<(): Invalid key '%d'.", key);
	}

	m_setKeyPool.insert(key);

	return *this;
}
