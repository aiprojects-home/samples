#pragma once

#include "XGlobals.h"

class XAudioVoice;

class XVoiceObject : IXAudio2VoiceCallback
{
private:
	WAVEFORMATEX           m_Format;
	IXAudio2SourceVoice   *m_pVoice;
	IXAudio2VoiceCallback *m_pExternalCallback;

public:

	XVoiceObject() = delete;
	XVoiceObject(XVoiceObject&) = delete;
	XVoiceObject(XVoiceObject&&) = delete;

	XVoiceObject(const WAVEFORMATEX& wfex);
	~XVoiceObject();

	operator IXAudio2SourceVoice*();

	void SetCallback(IXAudio2VoiceCallback* pCallack);

	void Reset();

	bool IsEqual(const WAVEFORMATEX& wfex) const;

private:
	// IXAudio2VoiceCallback для обратных вызовов.

	void OnStreamEnd();
	void OnVoiceProcessingPassEnd();
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired);
	void OnBufferEnd(void * pBufferContext);
	void OnBufferStart(void * pBufferContext);
	void OnLoopEnd(void * pBufferContext);
	void OnVoiceError(void * pBufferContext, HRESULT Error);
};

