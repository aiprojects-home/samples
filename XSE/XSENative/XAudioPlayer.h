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

	// ћьютексы дл€ блокировок.
	mutable std::shared_mutex m_MainLock;         // дл€ всего класса
	mutable std::mutex        m_CreateLock;       // дл€ очереди на создание
	mutable std::mutex        m_DeleteLock;       // дл€ очереди на удаление
	mutable std::mutex        m_CreateStreamLock; // дл€ очереди на создание потока
	mutable std::mutex        m_DeleteStreamLock; // дл€ очереди на удаление потока
	mutable std::mutex        m_VoicesLock;       // дл€ голосов
	mutable std::mutex        m_StreamVoicesLock; // дл€ потоковых голосов
	mutable std::mutex        m_NotifyLock;       // дл€ очереди уведомлений

	// —лужебные потоки.
	std::unique_ptr<std::thread> m_spCreatorThread;
	std::unique_ptr<std::thread> m_spDeleterThread;
	std::unique_ptr<std::thread> m_spCreatorStreamThread;
	std::unique_ptr<std::thread> m_spDeleterStreamThread;
	std::unique_ptr<std::thread> m_spNotifyThread;

	// CV дл€ ожидани€ служебных потоков.
	std::condition_variable m_cvCreate;
	std::condition_variable m_cvDelete;
	std::condition_variable m_cvCreateStream;
	std::condition_variable m_cvDeleteStream;
	std::condition_variable m_cvNotify;

	// ќчереди (создание / удаление / уведомлени€).
	std::queue<std::shared_ptr<XAudioVoice>> m_CreateQueue;
	std::queue<XAudioVoice*>                 m_DeleteQueue;

	std::queue<std::shared_ptr<XAudioVoice>> m_CreateStreamQueue;
	std::queue<XAudioVoice*>                 m_DeleteStreamQueue;

	std::queue<XNotifyMessage>               m_NotifyQueue;

	// √олоса (те что играют в насто€щее врем€).
	std::list<std::shared_ptr<XAudioVoice>>  m_Voices;
	std::list<std::shared_ptr<XAudioVoice>>  m_StreamVoices;

	// «вуковой банк.
	XSoundBank *m_pSoundBank;

	// √олосовой пул.
	XVoicePool m_VoicePool;

	// ‘лаги.
	std::atomic<bool> m_bTerminateFlag;

	// —осто€ние выполнени€.
	std::atomic<bool> m_bRunning;

	// ћаксимальное число голосов дл€ воспроизведени€.
	uint8_t m_nMaxVoices;
	uint8_t m_nMaxStreamVoices;

	// ”казатель на внешний интерфейс дл€ уведомлений.
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

