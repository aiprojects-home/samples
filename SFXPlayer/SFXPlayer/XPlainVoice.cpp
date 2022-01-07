#include "XPlainVoice.h"
#include "XSoundBank.h"
#include "XAudioPlayer.h"

XPlainVoice::XPlainVoice(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id)
	: XVoiceBase(audioPlayer, voicePool, soundBank, wfex, id)
{
	m_bInit = false;
}

XPlainVoice::~XPlainVoice()
{
	// Adjust counter.

	if (m_bInit)
	{
		m_refSoundBank.Release(m_nVoiceId);
	};
}

void XPlainVoice::FetchData()
{
	m_bInit = m_refSoundBank.Fetch(m_nVoiceId, m_Format, m_pBuffer, m_nBufferSize);

	if (!m_bInit)
	{
		throw XException(L"XPlainVoice::FetchData(): unable to fetch samples from soundbank");
	}
}

void XPlainVoice::SubmitAndPlay()
{
	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	XAUDIO2_BUFFER buffer;
	HRESULT hr;

	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = m_nBufferSize;
	buffer.pAudioData = m_pBuffer;
	buffer.PlayBegin = 0;
	buffer.PlayLength = 0;
	buffer.LoopBegin = 0;
	buffer.LoopLength = 0;
	buffer.LoopCount = 0;
	buffer.pContext = 0;

	// Try to submit.
	if (FAILED(hr = pVoice->SubmitSourceBuffer(&buffer)))
	{
		throw XException(L"XPlainVoice::SubmitAndPlay(): unable to submit source buffer");
	}

	pVoice->Start(0);
}

void XPlainVoice::OnStreamEnd()
{
	// Playing is finished. Add our voice to queue.

	{
		std::unique_lock lock{ m_refAudioPlayer.m_DeleteLock };

		m_refAudioPlayer.m_DeleteQueue.push(this);
	}

	m_refAudioPlayer.m_cvDelete.notify_one();

}
