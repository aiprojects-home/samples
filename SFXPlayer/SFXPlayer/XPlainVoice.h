#pragma once

#include "framework.h"
#include "XVoiceBase.h"

class XPlainVoice : public XVoiceBase
{
private:
	uint8_t  *m_pBuffer;
	uint32_t  m_nBufferSize;
	bool      m_bInit;

public:

	XPlainVoice() = delete;
	XPlainVoice(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id);
	~XPlainVoice();

	void FetchData();
	void SubmitAndPlay();

	virtual void OnStreamEnd();
};

