#pragma once

#include <memory>
#include <xaudio2.h>

class XAudioCore
{
private:
	static std::unique_ptr<XAudioCore> m_spCurrent;
	
	IXAudio2               *m_pXAudio2;
	IXAudio2MasteringVoice *m_pXAudio2MV;

	XAudioCore();

public:
	~XAudioCore();

	// Returns reference to the only copy of class object.
	static XAudioCore& Current();

	// Initializes XAudio2.
	void Init();

	// Destroys XAudio2 object & master voice.
	void Done();

	operator IXAudio2*();

	operator IXAudio2MasteringVoice*();
};

