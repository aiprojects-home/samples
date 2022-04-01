#include "XAudioVoice.h"
#include "XSoundBank.h"
#include "XVoicePool.h"
#include "XSoundDecoder.h"
#include "XAudioPlayer.h"

XAudioVoice::XAudioVoice()
{
	m_pSamples = nullptr;
	m_bPlaying = false;
}

XAudioVoice::~XAudioVoice()
{
	Stop();

	if (m_spVoice != nullptr)
	{
		// ���������� ����� � ���.
		m_pVoicePool->Put(m_spVoice);
	}

	if (m_pSamples)
	{
		// ���������� �������.
		m_pDecoder->FreeData();
	}
}

void XAudioVoice::Init(uint16_t nId, XAudioPlayer* pAudioPlayer, XVoicePool* pVoicePool, XSoundBank* pSoundBank)
{
	m_nVoiceId = nId;
	m_pAudioPlayer = pAudioPlayer;
	m_pVoicePool = pVoicePool;
	m_pSoundBank = pSoundBank;

	// �������� ������ ������ ��� ������.
	m_pDecoder = m_pSoundBank->GetDecoder(m_nVoiceId);

	if (!m_pDecoder)
	{
		throw XException(L"XAudioVoice::Init(): can't get decoder for id = '%d'", m_nVoiceId);
	}

	m_pDecoder->GetFormat(m_Format);

	// �������� ��������� ������ �� ���� ��� ������ �������.
	try
	{
		m_spVoice = m_pVoicePool->Get(m_Format);
		m_spVoice->SetCallback(this);
	} 
	catch (XException& e)
	{
		UNREFERENCED_PARAMETER(e);

		throw XException(L"XAudioVoice::Init(): can't get voice object from the pool for id = '%d'", m_nVoiceId);
	}

	m_bInitFlag = true;
}

uint16_t XAudioVoice::GetId()
{
	return m_nVoiceId;
}

void XAudioVoice::Play(bool bStreaming)
{
	if (!bStreaming)
	{
		// ������� ���������������.

		if (!m_pDecoder->IsLoaded())
		{
			// ������ ���� ��������� � ������.

			if (!m_pSoundBank->ReserveMemory(m_nVoiceId))
			{
				// ������ �� ������� -- �������, ��������������� ����������.
				throw XException(L"XAudioVoice::Play(): not enough memory for playing id = '%d'", m_nVoiceId);
			}

			// ������.
			if (!m_pDecoder->Load())
			{
				// ������-�� �� ����������, ���������� ����������������� ������ � ���� � �������.
				m_pSoundBank->FreeMemory(m_nVoiceId);

				throw XException(L"XAudioVoice::Play(): can't load data for id = '%d'", m_nVoiceId);
			}
		}

		// �������� ������.
		m_pDecoder->GetDataDirect(m_pSamples);

		// ������� ����� � �������� ���������������.

		IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

		XAUDIO2_BUFFER buffer;
		HRESULT hr;

		buffer.Flags = XAUDIO2_END_OF_STREAM;
		buffer.AudioBytes = m_pDecoder->GetSize();
		buffer.pAudioData = m_pSamples;
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopLength = 0;
		buffer.LoopCount = 0;
		buffer.pContext = 0;

		if (FAILED(hr = pVoice->SubmitSourceBuffer(&buffer)))
		{
			throw XException(L"XAudioVoice::Play(): unable to submit source buffer for id = '%d'", m_nVoiceId);
		}

		pVoice->Start(0);

	}
	else
	{
		// ��������� ���������������.
	    
		try
		{
			m_pDecoder->DecodeStart(m_Format);
		} 
		catch (XException& e)
		{
			UNREFERENCED_PARAMETER(e);

			throw XException(L"XAudioVoice::Play(): unable to start decoding for id = '%d'", m_nVoiceId);
		}

		// ����������� ������ ��� ������.

		m_nBufferSize = m_Format.nAvgBytesPerSec;

		for (uint8_t i = 0; i < 3; ++i)
		{
			m_upBuffer[i].reset(new BYTE[m_nBufferSize]);
		};

		m_nBufferToWrite = 0;
		m_nBuffersLeft = 0;

		// ���������� ������, ����� ��������� ��� ������. ������ ����� �� �������.
		m_bEOS = false;

		SubmitBuffer();
		SubmitBuffer();

		// �������� ����� ��� �������������.
		m_bUnloadFlag = false;
		m_bSubmitFlag = false;

		m_spDecoderThread.reset(new std::thread(&XAudioVoice::ThreadStreamDecoder, this));

		// OK.

		m_bPlaying = true;

		IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

		pVoice->Start();

	}
}

void XAudioVoice::Stop()
{
	if (!m_bPlaying)
	{
		// ������ ��� ���������� ���������������.
		return;
	}

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	pVoice->Stop();
	m_pDecoder->DecodeStop();

	// ��������� �����.

	m_bUnloadFlag = true;
	m_cvDecoder.notify_one();
	m_spDecoderThread->join();
	m_spDecoderThread = nullptr;

	m_bPlaying = false;

}

void XAudioVoice::SubmitBuffer()
{
	if (m_bEOS)
	{
		// ������ ���.
		return;
	}

	uint32_t nBytesRead = m_pDecoder->DecodeBytes(m_upBuffer[m_nBufferToWrite], m_nBufferSize);

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;
	XAUDIO2_BUFFER       buffer;

	if (nBytesRead)
	{
		buffer.Flags = 0;
		buffer.AudioBytes = nBytesRead;
		buffer.pAudioData = m_upBuffer[m_nBufferToWrite].get();
		buffer.PlayBegin = 0;
		buffer.PlayLength = 0;
		buffer.LoopBegin = 0;
		buffer.LoopLength = 0;
		buffer.LoopCount = 0;
		buffer.pContext = 0;

		// �������� ��������� �� ���������������.

		if (HRESULT hr; FAILED(hr = pVoice->SubmitSourceBuffer(&buffer)))
		{
			return;
		}

		m_nBuffersLeft++;
		m_nBufferToWrite = (m_nBufferToWrite + 1) % 3;
	};

	m_bEOS = !((bool)nBytesRead);
}

bool PredStreamDecoder(XAudioVoice& pObject)
{
	return (pObject.m_bUnloadFlag || pObject.m_bSubmitFlag);
}

void XAudioVoice::ThreadStreamDecoder()
{
	std::unique_lock lock{ m_DecoderLock };

	XAudioVoice& p = *this;

	for (;;)
	{
		m_cvDecoder.wait(lock, [&p]() {return PredStreamDecoder(p); });

		if (m_bUnloadFlag)
		{
			// ���� �����������.
			return;
		}

		if (m_bSubmitFlag)
		{
			// ������������ ����� ������.
			SubmitBuffer();

			m_bSubmitFlag = false;
		}
	}
}

void XAudioVoice::OnStreamEnd()
{
	if (m_spDecoderThread == nullptr)
	{
		// ������� ��������������� �����������. ��������� ���� � ������� �� ��������.

		{
			m_pAudioPlayer->OnPlaybackComplete(this, false);
		}
	}
	else
	{
		// ��� ���������� -- ������ �� ������.
	}
}

void XAudioVoice::OnVoiceProcessingPassEnd()
{

}

void XAudioVoice::OnVoiceProcessingPassStart(UINT32 SamplesRequired)
{

}

void XAudioVoice::OnBufferEnd(void * pBufferContext)
{
	if (m_spDecoderThread != nullptr)
	{
		// ��������� ��������������� ������ �����������. ���� ���-�� ������.

	   // ���������� ��������� ������ ������� (???).
		m_nBuffersLeft--;

		if (!m_nBuffersLeft)
		{
			// ��������� ����� �������� ���������������. �������.
			Stop();

			m_pAudioPlayer->OnPlaybackComplete(this, true);
		}
		else
		{
			// ����� �� ��� ��������� -- ������ ������.
			m_bSubmitFlag = true;
			m_cvDecoder.notify_one();
		}
	}
	else
	{
		// ��� �������� -- ������ �� ������.
	}
}

void XAudioVoice::OnBufferStart(void * pBufferContext)
{

}
void XAudioVoice::OnLoopEnd(void * pBufferContext)
{

}
void XAudioVoice::OnVoiceError(void * pBufferContext, HRESULT Error)
{

}
