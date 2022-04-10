#include "pch.h"
#include "XAudioVoice.h"
#include "XSoundBank.h"
#include "XVoicePool.h"
#include "XSoundDecoder.h"
#include "XAudioPlayer.h"
#include "XVoiceObject.h"
#include "XAudioCore.h"

XAudioVoice::XAudioVoice()
{
	m_pSamples = nullptr;
	m_bPlaying = false;

	m_fVolume = 1.0f;
	m_fPan = 0.0f;
}

XAudioVoice::~XAudioVoice()
{
	Stop();

	if (m_spVoice != nullptr)
	{
		// Возвращаем голос в пул.
		m_pVoicePool->Put(m_spVoice);
	}

	if (m_pSamples)
	{
		// Скручиваем счетчик.
		m_pDecoder->FreeData();
	}
}

void XAudioVoice::Init(uint16_t nId, XAudioPlayer* pAudioPlayer, XVoicePool* pVoicePool, XSoundBank* pSoundBank)
{
	m_nVoiceId = nId;
	m_pAudioPlayer = pAudioPlayer;
	m_pVoicePool = pVoicePool;
	m_pSoundBank = pSoundBank;

	// Получаем формат данных для голоса.
	m_pDecoder = m_pSoundBank->GetDecoder(m_nVoiceId);

	if (!m_pDecoder)
	{
		throw XException(L"XAudioVoice::Init(): can't get decoder for id = '%d'", m_nVoiceId);
	}

	m_pDecoder->GetFormat(m_Format);

	// Получаем голосовой объект из пула для нашего формата.
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

	// Устанавливаем общую громкость.

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	pVoice->SetVolume(m_fVolume);

	// Если голос - моно, устанавливаем панорамирование.

	if (m_Format.nChannels == 1)
	{
		auto &Core = XAudioCore::Current();
		auto *pMasterVoice = (IXAudio2MasteringVoice*)Core;

		DWORD dwChannelMask;
		pMasterVoice->GetChannelMask(&dwChannelMask);

		float outputMatrix[8] {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
		float left = 0.5f - m_fPan / 2.0f;
		float right = 0.5f + m_fPan / 2.0f;

		switch (dwChannelMask)
		{
		case KSAUDIO_SPEAKER_MONO:
			outputMatrix[0] = 1.0;
			break;
		case KSAUDIO_SPEAKER_STEREO:
		case KSAUDIO_SPEAKER_2POINT1:
		case KSAUDIO_SPEAKER_SURROUND:
			outputMatrix[0] = left;
			outputMatrix[1] = right;
			break;
		case KSAUDIO_SPEAKER_QUAD:
			outputMatrix[0] = outputMatrix[2] = left;
			outputMatrix[1] = outputMatrix[3] = right;
			break;
		case KSAUDIO_SPEAKER_5POINT1:
		case KSAUDIO_SPEAKER_7POINT1:
		case KSAUDIO_SPEAKER_5POINT1_SURROUND:
			outputMatrix[0] = outputMatrix[4] = left;
			outputMatrix[1] = outputMatrix[5] = right;
			break;
		case KSAUDIO_SPEAKER_7POINT1_SURROUND:
			outputMatrix[0] = outputMatrix[4] = outputMatrix[6] = left;
			outputMatrix[1] = outputMatrix[5] = outputMatrix[7] = right;
			break;
		}

		XAUDIO2_VOICE_DETAILS VoiceDetails;
		pVoice->GetVoiceDetails(&VoiceDetails);

		XAUDIO2_VOICE_DETAILS MasterVoiceDetails;
		pMasterVoice->GetVoiceDetails(&MasterVoiceDetails);

		pVoice->SetOutputMatrix(NULL, VoiceDetails.InputChannels, MasterVoiceDetails.InputChannels, outputMatrix);
	};

	m_bInitFlag = true;
}

void XAudioVoice::SetVolume(float fVolume)
{
	m_fVolume = fVolume;
}

void XAudioVoice::SetPan(float fPan)
{
	m_fPan = fPan;
}

uint16_t XAudioVoice::GetId() const
{
	return m_nVoiceId;
}

void XAudioVoice::Play(bool bStreaming)
{
	if (!bStreaming)
	{
		// Обычное воспроизведение.

		if (!m_pDecoder->IsLoaded())
		{
			// Семплы надо загружать в память.

			if (!m_pSoundBank->ReserveMemory(m_nVoiceId))
			{
				// Памяти не хватает -- выходим, воспроизведение невозможно.
				throw XException(L"XAudioVoice::Play(): not enough memory for playing id = '%d'", m_nVoiceId);
			}

			// Грузим.
			if (!m_pDecoder->Load())
			{
				// Почему-то не загрузился, возвращаем зарезервированную память в банк и выходим.
				m_pSoundBank->FreeMemory(m_nVoiceId);

				throw XException(L"XAudioVoice::Play(): can't load data for id = '%d'", m_nVoiceId);
			}
		}

		// Получаем семплы.
		m_pDecoder->GetDataDirect(m_pSamples);

		// Создаем буфер и начинаем воспроизведение.

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
		// Потоковое воспроизведение.
	    
		try
		{
			m_pDecoder->DecodeStart();
		} 
		catch (XException& e)
		{
			UNREFERENCED_PARAMETER(e);

			throw XException(L"XAudioVoice::Play(): unable to start decoding for id = '%d'", m_nVoiceId);
		}

		// Резервируем память под буферы.

		m_nBufferSize = m_Format.nAvgBytesPerSec;

		for (uint8_t i = 0; i < 3; ++i)
		{
			m_upBuffer[i].reset(new BYTE[m_nBufferSize]);
		};

		m_nBufferToWrite = 0;
		m_nBuffersLeft = 0;

		// Декодируем дважды, чтобы наполнить оба буфера. Третий будет на очереди.
		m_bEOS = false;

		SubmitBuffer();
		SubmitBuffer();

		// Стартуем поток для декодирования.
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
		// Только для потокового воспроизведения.
		return;
	}

	IXAudio2SourceVoice *pVoice = (IXAudio2SourceVoice *)(*m_spVoice.get());;

	pVoice->Stop();
	m_pDecoder->DecodeStop();

	// Выгружаем поток.

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
		// Данных нет.
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

		// Пытаемся отправить на воспроизведение.

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
			// Пора выгружаться.
			return;
		}

		if (m_bSubmitFlag)
		{
			// Рапаковываем новую порцию.
			SubmitBuffer();

			m_bSubmitFlag = false;
		}
	}
}

void XAudioVoice::OnStreamEnd()
{
	if (m_spDecoderThread == nullptr)
	{
		// Обычное воспроизведение закончилось. Добавляем себя в очередь на удаление.

		m_pAudioPlayer->OnPlaybackComplete(this, false);
	}
	else
	{
		// Для потокового -- ничего не делаем.
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
		// Потоковое воспроизведение буфера закончилось. Надо что-то делать.

	   // Декодируем следующую порцию семплов (???).
		m_nBuffersLeft--;

		if (!m_nBuffersLeft)
		{
			// Последний буфер закончил воспроизведение. Выходим.
			Stop();

			m_pAudioPlayer->OnPlaybackComplete(this, true);
		}
		else
		{
			// Буфер не был последним -- качаем дальше.
			m_bSubmitFlag = true;
			m_cvDecoder.notify_one();
		}
	}
	else
	{
		// Для обычного -- ничего не делаем.
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
