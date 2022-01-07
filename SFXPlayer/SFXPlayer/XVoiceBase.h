#pragma once

#include "framework.h"
#include "XVoiceObject.h"
#include <memory>

class XAudioPlayer;
class XSoundBank;
class XVoicePool;
class XVoiceBase
{
protected:
	XAudioPlayer& m_refAudioPlayer;
	XSoundBank&   m_refSoundBank;
	XVoicePool&   m_refVoicePool;
	WAVEFORMATEX  m_Format;
	
	std::shared_ptr<XVoiceObject> m_spVoice;

public:

	uint16_t      m_nVoiceId;

	XVoiceBase() = delete;
	XVoiceBase(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id);
	~XVoiceBase();

	void Init();

	// Callbacks.
	virtual void OnStreamEnd();
	virtual void OnVoiceProcessingPassEnd();
	virtual void OnVoiceProcessingPassStart(UINT32 SamplesRequired);
	virtual void OnBufferEnd(void * pBufferContext);
	virtual void OnBufferStart(void * pBufferContext);
	virtual void OnLoopEnd(void * pBufferContext);
	virtual void OnVoiceError(void * pBufferContext, HRESULT Error);
};

