#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <atomic>
#include <xaudio2.h>
#include <any>

#include "XSoundBank.h"
#include "XVoicePool.h"
#include "XVoiceBase.h"
#include "XPlainVoice.h"
#include "XStreamVoice.h"

// External callback.
class IXAudioPlayerCallback
{
public:

	// Called when streaming voice finishes playing (not when manually stopped, but reaches the end of data).
	virtual void OnStreamingEnd(uint16_t id) = 0;

	// Called when error occured.
	virtual void OnError(const uint16_t id, const XException e) = 0;
};

class XAudioPlayer
{
	friend class XVoiceBase;
	friend class XPlainVoice;
	friend class XStreamVoice;

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
	mutable std::shared_mutex m_MainLock;         // protects class
	mutable std::mutex        m_CreateLock;       // protects creation queue
	mutable std::mutex        m_DeleteLock;       // protects removing queue
	mutable std::mutex        m_CreateStreamLock; // protects creation queue for streaming
	mutable std::mutex        m_DeleteStreamLock; // protects removing queue for streaming
	mutable std::mutex        m_VoicesLock;       // protects playing list
	mutable std::mutex        m_StreamVoicesLock; // protects playing list of streaming voices

	mutable std::mutex        m_NotifyLock;       // protects notify queue

	// Helper threads.
	std::unique_ptr<std::thread> m_spCreatorThread;
	std::unique_ptr<std::thread> m_spDeleterThread;
	std::unique_ptr<std::thread> m_spCreatorStreamThread;
	std::unique_ptr<std::thread> m_spDeleterStreamThread;
	std::unique_ptr<std::thread> m_spNotifyThread;

	// CV to notify working threads.
	std::condition_variable m_cvCreate;
	std::condition_variable m_cvDelete;
	std::condition_variable m_cvCreateStream;
	std::condition_variable m_cvDeleteStream;
	std::condition_variable m_cvNotify;

	// Queues (creation / deleting / notify).
	std::queue<std::shared_ptr<XPlainVoice>> m_CreateQueue;
	std::queue<XPlainVoice*>                 m_DeleteQueue;

	std::queue<std::shared_ptr<XStreamVoice>> m_CreateStreamQueue;
	std::queue<XStreamVoice*>                 m_DeleteStreamQueue;

	std::queue<XNotifyMessage> m_NotifyQueue;

	// Voices list (sounds that are currently playing).
	std::list<std::shared_ptr<XPlainVoice>>   m_Voices;
	std::list<std::shared_ptr<XStreamVoice>>  m_StreamVoices;

	// Attached soundbank.
	XSoundBank *m_pSoundBank;

	// Voice pool.
	XVoicePool m_VoicePool;

	// Flags.
	std::atomic<bool> m_bTerminateFlag;

	// Running condition.
	std::atomic<bool> m_bRunning;

	// Max count of voices playing.
	uint8_t m_nMaxVoices;
	uint8_t m_nMaxStreamVoices;

	// Sounds counter.
	std::atomic<uint32_t> m_nSC;

	// Callback pointer.
	IXAudioPlayerCallback *m_pCallback;

public:

	XAudioPlayer();
	~XAudioPlayer();

	void AttachSoundBank(XSoundBank& bank);

	void SetMaxVoices(const uint8_t count);

	uint8_t GetMaxVoices() const;

	void SetMaxStreamVoices(const uint8_t count);

	uint8_t GetMaxStreamVoices() const;

	void SetCallback(IXAudioPlayerCallback *pCallback);

	void Start();
	void Stop();

	void PlaySimple(const uint16_t id);

	void PlayStream(const uint16_t id);

	void StopStream(const uint16_t id);

private:
	friend bool PredCreator(XAudioPlayer& pObject);
	friend bool PredDeleter(XAudioPlayer& pObject);
	friend bool PredStreamCreator(XAudioPlayer& pObject);
	friend bool PredStreamDeleter(XAudioPlayer& pObject);

	friend bool PredStreamDecoder(XStreamVoice& pObject);

	friend bool PredNotifier(XAudioPlayer& pObject);

	void ThreadCreator();
	void ThreadDeleter();
	void ThreadStreamCreator();
	void ThreadStreamDeleter();

	void ThreadNotifier();

	void PostNotify(XNotifyMessage msg);
};

