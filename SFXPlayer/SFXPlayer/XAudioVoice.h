#pragma once

#include "framework.h"
#include <memory>
#include <thread>
#include <condition_variable>
#include <atomic>

class XAudioPlayer;
class XVoicePool;
class XSoundBank;
class XVoiceObject;
class XSoundDecoder;

class XAudioVoice : public IXAudio2VoiceCallback
{
	friend bool PredStreamDecoder(XAudioVoice& pObject);

private:

	// ������:

	XAudioPlayer*                 m_pAudioPlayer;    // ��������� �� ����� ������
	XSoundBank*                   m_pSoundBank;      // ��������� �� �������� ����
	XVoicePool*                   m_pVoicePool;      // ��������� �� ��� �������
	WAVEFORMATEX                  m_Format;          // ������ �������
	std::shared_ptr<XVoiceObject> m_spVoice;         // ��������� �� XAudio2 Voice �� ����
    uint16_t                      m_nVoiceId;        // ������������� ������ � �����
	std::unique_ptr<BYTE[]>       m_upBuffer[3];     // ������� ����� ��� ���������� ���������������
	uint32_t                      m_nBufferSize;     // ������ ������
	uint8_t                       m_nBufferToWrite;  // ����� ������ ��� �������� �� ���������������
	std::atomic<uint8_t>          m_nBuffersLeft;    // ���������� ���������� �������
	std::mutex                    m_DecoderLock;     // ���������� ��� ��������
	std::condition_variable       m_cvDecoder;       // ���������� ��� �������� �������������
	std::unique_ptr<std::thread>  m_spDecoderThread; // ����� �������������
	XSoundDecoder*                m_pDecoder;        // ��������� �� �������
	uint8_t*                      m_pSamples;        // ��������� �� ������

	// �����:
	bool m_bEOS;        // ������ ���������
	bool m_bUnloadFlag; // ���� ��������� ����� �������������
	bool m_bSubmitFlag; // ���� ������������ ������ � ��������� �����
	bool m_bPlaying;    // ���� ���������� ���������������
	bool m_bInitFlag;   // ���� �������������

public:

	XAudioVoice();
	~XAudioVoice();

	void Init(uint16_t nId, XAudioPlayer* pAudioPlayer, XVoicePool* pVoicePool, XSoundBank* pSoundBank);
	
	uint16_t GetId();
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

