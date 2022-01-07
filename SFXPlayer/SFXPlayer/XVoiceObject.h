#pragma once

#include "framework.h"

class XVoiceBase;

class XVoiceObject : IXAudio2VoiceCallback
{
private:
	WAVEFORMATEX         m_Format;
	IXAudio2SourceVoice *m_pVoice;
	XVoiceBase          *m_pOwner;

public:

	XVoiceObject() = delete;
	XVoiceObject(const WAVEFORMATEX& wfex);
	~XVoiceObject();

	operator IXAudio2SourceVoice*();

	void SetOwner(XVoiceBase* pOwner);

	void Reset();

	bool IsEqual(const WAVEFORMATEX& wfex);

private:
	// IXAudio2VoiceCallback part
	void OnStreamEnd();
	void OnVoiceProcessingPassEnd();
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired);
	void OnBufferEnd(void * pBufferContext);
	void OnBufferStart(void * pBufferContext);
	void OnLoopEnd(void * pBufferContext);
	void OnVoiceError(void * pBufferContext, HRESULT Error);
};

