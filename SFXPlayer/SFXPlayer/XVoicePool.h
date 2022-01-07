#pragma once

#include "XVoiceObject.h"
#include "XException.h"
#include "framework.h"

#include <list>
#include <memory>
#include <mutex>

class XVoicePool
{
private:
	uint32_t                                 m_nMaxVoices;
	std::list<std::shared_ptr<XVoiceObject>> m_spPool;
	std::mutex                               m_Lock;

public:

	XVoicePool() = delete;
	XVoicePool(uint32_t nMaxVoices = 32);
	~XVoicePool();

	std::shared_ptr<XVoiceObject> Get(const WAVEFORMATEX& wfex);
	void Put(const std::shared_ptr<XVoiceObject>& spVoice);

	void Clear();
};

