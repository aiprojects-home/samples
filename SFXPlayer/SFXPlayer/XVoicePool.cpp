#include "XVoicePool.h"

XVoicePool::XVoicePool(uint32_t nMaxVoices)
{
	m_nMaxVoices = nMaxVoices;
}

XVoicePool::~XVoicePool()
{

}

std::shared_ptr<XVoiceObject> XVoicePool::Get(const WAVEFORMATEX& wfex)
{
	std::unique_lock lock{ m_Lock };

	// Searching for requested voice.

	for(auto it = m_spPool.begin(); it != m_spPool.end(); ++it)
	{
		auto ptr = *it;
		if (ptr->IsEqual(wfex))
		{
			// Found.
			m_spPool.erase(it);
			return ptr;
		}
	}

	// Not found. Try to create new.
	std::shared_ptr<XVoiceObject> spNewVoice;

	try
	{
		spNewVoice = std::make_shared<XVoiceObject>(wfex);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XVoicePool::Get(): can't create requested voice");
	}

	return spNewVoice;
}

void XVoicePool::Put(const std::shared_ptr<XVoiceObject>& spVoice)
{
	std::unique_lock lock{ m_Lock };

	if (m_spPool.size() >= m_nMaxVoices)
	{
		// Push out last voice -- we already have the maximum number.
		m_spPool.resize(m_nMaxVoices - 1);
	};

	spVoice->Reset();
	m_spPool.push_front(spVoice);
}

void XVoicePool::Clear()
{
	std::unique_lock lock{ m_Lock };

	m_spPool.clear();
}
