#pragma once

#include "XGlobals.h"

class XAudioPlayer;
class XVoicePool;
class XSoundBank;
class XVoiceObject;
class XSoundDecoder;

class XAudioVoice : public IXAudio2VoiceCallback
{
	friend bool PredStreamDecoder(XAudioVoice& pObject);

private:

	// Данные:

	XAudioPlayer*                 m_pAudioPlayer;    // указатель на класс плеера
	XSoundBank*                   m_pSoundBank;      // указатель на звуковой банк
	XVoicePool*                   m_pVoicePool;      // указатель на пул голосов
	WAVEFORMATEX                  m_Format;          // формат семплов
	std::shared_ptr<XVoiceObject> m_spVoice;         // указатель на XAudio2 Voice из пула
    uint16_t                      m_nVoiceId;        // идентификатор голоса в банке
	std::unique_ptr<BYTE[]>       m_upBuffer[3];     // тройной буфер для потокового воспроизведения
	uint32_t                      m_nBufferSize;     // размер буфера
	uint8_t                       m_nBufferToWrite;  // номер буфера для отправки на воспроизведение
	std::atomic<uint8_t>          m_nBuffersLeft;    // количество оставшихся буферов
	std::mutex                    m_DecoderLock;     // блокировка для декодера
	std::condition_variable       m_cvDecoder;       // переменная для ожидания декодирования
	std::unique_ptr<std::thread>  m_spDecoderThread; // поток декодирования
	XSoundDecoder*                m_pDecoder;        // указатель на декодер
	uint8_t*                      m_pSamples;        // указатель на семплы

	// Флаги:
	bool m_bEOS;        // семплы кончились
	bool m_bUnloadFlag; // пора выгружать поток декодирования
	bool m_bSubmitFlag; // пора декодировать семплы в очередной буфер
	bool m_bPlaying;    // флаг потокового воспроизведения
	bool m_bInitFlag;   // флаг инициализации

	// Доп. настройки голоса:
	float m_fVolume;    // громкость
	float m_fPan;       // панорамирование (только для моно)

public:

	XAudioVoice();
	XAudioVoice(XAudioVoice&) = delete;
	XAudioVoice(XAudioVoice&&) = delete;
	~XAudioVoice();

	void Init(uint16_t nId, XAudioPlayer* pAudioPlayer, XVoicePool* pVoicePool, XSoundBank* pSoundBank);
	
	void SetVolume(float fVolume);

	void SetPan(float fPan);

	uint16_t GetId() const;
	void Play(bool bStreaming);
	void Stop();

public:

	// Callbacks.
	void OnStreamEnd();
	void OnVoiceProcessingPassEnd();
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired);
	void OnBufferEnd(void * pBufferContext);
	void OnBufferStart(void * pBufferContext);
	void OnLoopEnd(void * pBufferContext);
	void OnVoiceError(void * pBufferContext, HRESULT Error);

private:

	void SubmitBuffer();
	void ThreadStreamDecoder();
};

