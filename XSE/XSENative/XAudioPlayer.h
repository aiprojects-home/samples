#pragma once

#include "XGlobals.h"
#include "XSoundBank.h"
#include "XVoicePool.h"
#include "XAudioVoice.h"
#include "IXAudioPlayerCallback.h"

class XAudioPlayer
{
private:

	enum class XPlayerNotify : uint8_t
	{
		XN_STREAMING_STOP,
		XN_ERROR
	};

	class XNotifyMessage
	{
	public:

		uint16_t       m_nVoiceId;
		XPlayerNotify  m_nMessageId;
		std::any       m_Extra;

	public:

		XNotifyMessage(uint16_t id, XPlayerNotify msg) : m_nVoiceId(id), m_nMessageId(msg) {};
		XNotifyMessage(uint16_t id, XPlayerNotify msg, std::any extra) : m_nVoiceId(id), m_nMessageId(msg), m_Extra(extra) {};
	};

private:

	// �������� ��� ����������.
	mutable std::shared_mutex m_MainLock;         // ��� ����� ������
	mutable std::mutex        m_CreateLock;       // ��� ������� �� ��������
	mutable std::mutex        m_DeleteLock;       // ��� ������� �� ��������
	mutable std::mutex        m_CreateStreamLock; // ��� ������� �� �������� ������
	mutable std::mutex        m_DeleteStreamLock; // ��� ������� �� �������� ������
	mutable std::mutex        m_VoicesLock;       // ��� �������
	mutable std::mutex        m_StreamVoicesLock; // ��� ��������� �������
	mutable std::mutex        m_NotifyLock;       // ��� ������� �����������

	// ��������� ������.
	std::unique_ptr<std::thread> m_spCreatorThread;
	std::unique_ptr<std::thread> m_spDeleterThread;
	std::unique_ptr<std::thread> m_spCreatorStreamThread;
	std::unique_ptr<std::thread> m_spDeleterStreamThread;
	std::unique_ptr<std::thread> m_spNotifyThread;

	// CV ��� �������� ��������� �������.
	std::condition_variable m_cvCreate;
	std::condition_variable m_cvDelete;
	std::condition_variable m_cvCreateStream;
	std::condition_variable m_cvDeleteStream;
	std::condition_variable m_cvNotify;

	// ������� (�������� / �������� / �����������).
	std::queue<std::shared_ptr<XAudioVoice>> m_CreateQueue;
	std::queue<XAudioVoice*>                 m_DeleteQueue;

	std::queue<std::shared_ptr<XAudioVoice>> m_CreateStreamQueue;
	std::queue<XAudioVoice*>                 m_DeleteStreamQueue;

	std::queue<XNotifyMessage>               m_NotifyQueue;

	// ������ (�� ��� ������ � ��������� �����).
	std::list<std::shared_ptr<XAudioVoice>>  m_Voices;
	std::list<std::shared_ptr<XAudioVoice>>  m_StreamVoices;

	// �������� ����.
	XSoundBank *m_pSoundBank;

	// ��������� ���.
	XVoicePool m_VoicePool;

	// �����.
	std::atomic<bool> m_bTerminateFlag;

	// ��������� ����������.
	std::atomic<bool> m_bRunning;

	// ������������ ����� ������� ��� ���������������.
	uint8_t m_nMaxVoices;
	uint8_t m_nMaxStreamVoices;

	// ��������� �� ������� ��������� ��� �����������.
	IXAudioPlayerCallback *m_pCallback;

public:

	XAudioPlayer();
	~XAudioPlayer();

	void AttachSoundBank(XSoundBank* pBank);

	void SetMaxVoices(const uint8_t count);

	uint8_t GetMaxVoices() const;

	void SetMaxStreamVoices(const uint8_t count);

	uint8_t GetMaxStreamVoices() const;

	void SetCallback(IXAudioPlayerCallback *pCallback);

	void Start();
	void Stop();

	void PlaySimple(const uint16_t nId, float fVolume = 1.0f, float fPan = 0.0f);

	void PlayStream(const uint16_t nId, float fVolume = 1.0f);

	void StopStream(const uint16_t nId);

	void StopAll();

	void OnPlaybackComplete(XAudioVoice* pVoice, bool bStream);

private:

	friend bool PredCreator(XAudioPlayer& pObject);
	friend bool PredDeleter(XAudioPlayer& pObject);
	friend bool PredStreamCreator(XAudioPlayer& pObject);
	friend bool PredStreamDeleter(XAudioPlayer& pObject);
	friend bool PredNotifier(XAudioPlayer& pObject);

	void ThreadCreator();
	void ThreadDeleter();
	void ThreadStreamCreator();
	void ThreadStreamDeleter();
	void ThreadNotifier();

	void PostNotify(XNotifyMessage msg);
};

