#pragma once

#include "XGlobals.h"

class XVoiceObject;

class XVoicePool
{
private:
	uint32_t                                 m_nMaxVoices;
	std::list<std::shared_ptr<XVoiceObject>> m_spPool;
	std::mutex                               m_Lock;

public:

	XVoicePool() = delete;
	XVoicePool(XVoicePool& other) = delete;
	XVoicePool(XVoicePool&& other) = delete;

	XVoicePool(uint32_t nMaxVoices = 32);
	~XVoicePool();

	std::shared_ptr<XVoiceObject> Get(const WAVEFORMATEX& wfex);
	void Put(const std::shared_ptr<XVoiceObject>& spVoice);

	void Clear();
};

