#include "XStreamVoice.h"
#include "XSoundBank.h"
#include "XAudioCore.h"
#include "XAudioPlayer.h"

XStreamVoice::XStreamVoice(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id)
	: XVoiceBase(audioPlayer, voicePool, soundBank, wfex, id)
{
	m_bPlaying = false;
}

XStreamVoice::~XStreamVoice()
{
	StopAndUnload();
}

void XStreamVoice::CreateAndPlay()
{
	// Preparing.
	m_refSoundBank.StartStreaming(m_nVoiceId, m_Format);

	// Obtaining voice from the pool.
	try
	{
		m_spVoice = m_refVoicePool.Get(m_Format);
	}
	catch (const XException& e)
	{
		m_refSoundBank.StopStreaming(m_nVoiceId);

		throw XException(e, L"XStreamVoice::CreateAndPlay(): unable to get requested voice from the pool");
	}

	m_spVoice->SetOwner(this);

	// Reserving memory for buffers.
	m_nBufferSize = m_Format.nAvgBytesPerSec;

	for (uint8_t i = 0; i < 3; ++i)
	{
		m_spBuffer[i].reset(new BYTE[m_nBufferSize]);
	};

	m_nBufferToWrite = 0;
	m_nBuffersLeft = 0;

	// Decoding twice to fill two buffers.
	m_bEOS = false;

	SubmitBuffer();
	SubmitBuffer();

	// Starting decoding thread.
	m_bUnloadFlag = false;
	m_bSubmitFlag = false;

	m_spDecoderThread.reset(new std::thread(&XStreamVoice::ThreadStreamDecoder, this));

	// OK.

	m_bPlaying = true;

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	pVoice->Start();
}

void XStreamVoice::StopAndUnload()
{
	if (!m_bPlaying)
	{
		return;
	}

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	pVoice->Stop();
	m_refSoundBank.StopStreaming(m_nVoiceId);

	// Unloading thread.
	m_bUnloadFlag = true;
	m_cvDecoder.notify_one();
	m_spDecoderThread->join();
	m_spDecoderThread = nullptr;

	m_bPlaying = false;
}

void XStreamVoice::OnBufferEnd(void * pBufferContext)
{
	// Decoding next part.
	m_nBuffersLeft--;

	if (!m_nBuffersLeft)
	{
		// Last buffer was played. Quit.
		StopAndUnload();

		{
			std::unique_lock qlock{ m_refAudioPlayer.m_DeleteStreamLock };

			// Adding ourselves to deleting queue.

			m_refAudioPlayer.m_DeleteStreamQueue.push(this);
		}

		m_refAudioPlayer.m_cvDeleteStream.notify_one();

		// Posting notify message.
		m_refAudioPlayer.PostNotify(XAudioPlayer::XNotifyMessage(m_nVoiceId, XAudioPlayer::XPlayerNotify::XN_STREAMING_STOP));

	}
	else
	{
		// Submit another one.
		m_bSubmitFlag = true;
		m_cvDecoder.notify_one();
	}

}

bool PredStreamDecoder(XStreamVoice& pObject)
{
	return (pObject.m_bUnloadFlag || pObject.m_bSubmitFlag);
}

void XStreamVoice::SubmitBuffer()
{
	if (m_bEOS)
	{
		// Out of data.
		return;
	}

	uint32_t nBytesRead = m_refSoundBank.GetStreamData(m_nVoiceId, m_spBuffer[m_nBufferToWrite], m_nBufferSize);

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;
	XAUDIO2_BUFFER       buffer;

	if (nBytesRead)
	{
		buffer.Flags = 0;
		buffer.AudioBytes = nBytesRead;
		buffer.pAudioData = m_spBuffer[m_nBufferToWrite].get();
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopLength = 0;
		buffer.LoopCount = 0;
		buffer.pContext = 0;

		// Try to submit.

		if (HRESULT hr; FAILED(hr = pVoice->SubmitSourceBuffer(&buffer)))
		{
			int a = 0;
			return;
		}

		m_nBuffersLeft++;
		m_nBufferToWrite = (m_nBufferToWrite + 1) % 3;
	};

	m_bEOS = !((bool)nBytesRead);
}

void XStreamVoice::ThreadStreamDecoder()
{
	std::unique_lock lock{ m_DecoderLock };

	XStreamVoice& p = *this;

	for (;;)
	{
		m_cvDecoder.wait(lock, [&p]() {return PredStreamDecoder(p); });

		if (m_bUnloadFlag)
		{
			// Unloading...
			return;
		}

		if (m_bSubmitFlag)
		{
			// Submit next buffer...
			SubmitBuffer();

			m_bSubmitFlag = false;
		}
	}
}
