#include "XVoiceBase.h"
#include "XVoicePool.h"

XVoiceBase::XVoiceBase(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id)
	: m_refAudioPlayer(audioPlayer), m_refVoicePool(voicePool), m_refSoundBank(soundBank)
{
	m_Format = wfex;
	m_nVoiceId = id;
}

XVoiceBase::~XVoiceBase()
{
	// Sending voice back to the pool.

	m_refVoicePool.Put(m_spVoice);
}

void XVoiceBase::Init()
{
	try
	{
		m_spVoice = m_refVoicePool.Get(m_Format);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XVoiceBase::Init(): unable to get requested voice from the pool");
	}

	m_spVoice->SetOwner(this);
}


void XVoiceBase::OnStreamEnd() {};
void XVoiceBase::OnVoiceProcessingPassEnd() {};
void XVoiceBase::OnVoiceProcessingPassStart(UINT32 SamplesRequired) {};
void XVoiceBase::OnBufferEnd(void * pBufferContext) {};
void XVoiceBase::OnBufferStart(void * pBufferContext) {};
void XVoiceBase::OnLoopEnd(void * pBufferContext) {};
void XVoiceBase::OnVoiceError(void * pBufferContext, HRESULT Error) {};
