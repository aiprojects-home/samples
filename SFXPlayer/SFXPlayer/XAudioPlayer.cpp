#include "XAudioPlayer.h"
#include "XException.h"
#include "XAux.h"
#include "XSoundBank.h"
#include "XAudioCore.h"
#include "XVoicePool.h"

XAudioPlayer::XAudioPlayer() : m_VoicePool(32)
{
	m_pSoundBank = nullptr;
	m_pCallback = nullptr;
	m_bRunning = false;
	m_nMaxVoices = 1;
	m_nMaxStreamVoices = 1;

	m_nSC = 0;
}

XAudioPlayer::~XAudioPlayer()
{
	Stop();

	uint32_t check = m_nSC.load(); // must be zero
}

void XAudioPlayer::AttachSoundBank(XSoundBank& bank)
{
	std::unique_lock lock{ m_MainLock };

	if (m_bRunning)
	{
		throw XException(L"XAudioPlayer::AttachSoundBank(): can't attach bank while running");
	}

	m_pSoundBank = &bank;
}

void XAudioPlayer::SetMaxVoices(const uint8_t count)
{
	std::unique_lock lock{ m_MainLock };
	std::unique_lock vlock{ m_VoicesLock }; // because working thread also can access m_nMaxVoices

	m_nMaxVoices = count;
}

uint8_t XAudioPlayer::GetMaxVoices() const
{
	std::shared_lock lock{ m_MainLock };

	return m_nMaxVoices;
}

void XAudioPlayer::SetMaxStreamVoices(const uint8_t count)
{
	std::unique_lock lock{ m_MainLock };

	m_nMaxStreamVoices = count;
}

uint8_t XAudioPlayer::GetMaxStreamVoices() const
{
	std::shared_lock lock{ m_MainLock };

	return m_nMaxStreamVoices;
}

void XAudioPlayer::SetCallback(IXAudioPlayerCallback *pCallback)
{
	std::unique_lock lock{ m_MainLock };
	std::unique_lock qlock{ m_NotifyLock }; // because working thread also calling through m_pCallback

	m_pCallback = pCallback;
}

void XAudioPlayer::Start()
{
	std::unique_lock lock{ m_MainLock };

	if (m_bRunning)
	{
		// Already running.
		return;
	}

	if (m_pSoundBank == nullptr)
	{
		throw XException(L"XAudioPlayer::Start(): soundbank is not attached");
	}

	// Clear flag & create threads.
	m_bTerminateFlag = false;
	m_spCreatorThread.reset(new std::thread(&XAudioPlayer::ThreadCreator, this));
	m_spDeleterThread.reset(new std::thread(&XAudioPlayer::ThreadDeleter, this));

	m_spCreatorStreamThread.reset(new std::thread(&XAudioPlayer::ThreadStreamCreator, this));
	m_spDeleterStreamThread.reset(new std::thread(&XAudioPlayer::ThreadStreamDeleter, this));

	m_spNotifyThread.reset(new std::thread(&XAudioPlayer::ThreadNotifier, this));

	m_bRunning = true;
}

void XAudioPlayer::Stop()
{
	std::unique_lock lock{ m_MainLock };

	if (!m_bRunning)
	{
		// Already stopped.
		return;
	}

	// Setup terminate flag and wait until working threads are finished.
	m_bTerminateFlag = true;

	m_cvCreate.notify_one();
	m_cvDelete.notify_one();

	m_cvCreateStream.notify_one();
	m_cvDeleteStream.notify_one();

	m_cvNotify.notify_one();

	m_spCreatorThread->join();
	m_spDeleterThread->join();

	m_spCreatorStreamThread->join();
	m_spDeleterStreamThread->join();

	m_spNotifyThread->join();

	m_spCreatorThread.reset(nullptr);
	m_spDeleterThread.reset(nullptr);

	m_spCreatorStreamThread.reset(nullptr);
	m_spDeleterStreamThread.reset(nullptr);

	m_spNotifyThread.reset(nullptr);

	// Clearing data.

	// 1. queues
	XAux::ClearAdapter(m_CreateQueue);
	XAux::ClearAdapter(m_CreateStreamQueue);
	XAux::ClearAdapter(m_DeleteQueue);
	XAux::ClearAdapter(m_DeleteStreamQueue);
	XAux::ClearAdapter(m_NotifyQueue);

	// 2. playing voices

	m_Voices.clear();
	m_StreamVoices.clear();

	// 3. Voice pool

	m_VoicePool.Clear();

	m_bRunning = false;
}

void XAudioPlayer::PlaySimple(const uint16_t id)
{
	std::unique_lock lock{ m_MainLock };

	if (!m_bRunning)
	{
		throw XException(L"XAudioPlayer::PlaySimple(): player is not running");
	}

	if (!m_pSoundBank->CanFetch(id))
	{
		throw XException(L"XAudioPlayer::PlaySimple(): can't fetch sound with id='%d'", id);
	}

	WAVEFORMATEX wfex;

	m_pSoundBank->FetchFormat(id, wfex);

	// Everything is OK. Add sound to queue.

	{
		std::unique_lock qlock{ m_CreateLock };

		std::shared_ptr<XPlainVoice> voice = std::make_shared<XPlainVoice>(*this, m_VoicePool, *m_pSoundBank, wfex, id);

		m_CreateQueue.push(voice);
	}

	m_cvCreate.notify_one();
}

void XAudioPlayer::PlayStream(const uint16_t id)
{
	std::unique_lock lock{ m_MainLock };

	if (!m_bRunning)
	{
		throw XException(L"XAudioPlayer::PlayStream(): player is not running");
	}

	// Checking how many streams are playing now.

	std::unique_lock vlock{ m_StreamVoicesLock };
	{
		if (m_StreamVoices.size() >= m_nMaxStreamVoices)
		{
			// Can't play more.
			return;
		}
	}

	if (!m_pSoundBank->CanStream(id))
	{
		throw XException(L"XAudioPlayer::PlayStream(): can't stream music with id='%d'", id);
	}

	// Everything is OK. Add stream to queue.

	WAVEFORMATEX wfex_dummy;

	{
		std::unique_lock qlock{ m_CreateStreamLock };

		std::shared_ptr<XStreamVoice> voice = std::make_shared<XStreamVoice>(*this, m_VoicePool, *m_pSoundBank, wfex_dummy, id);

		m_CreateStreamQueue.push(voice);
	}

	m_cvCreateStream.notify_one();
}

bool PredCreator(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_CreateQueue.size());
}

void XAudioPlayer::StopStream(const uint16_t id)
{
	std::unique_lock lock{ m_MainLock };

	{
		// Searching stream with given id.

		std::unique_lock vlock{ m_StreamVoicesLock };
		
		for (auto &ptr : m_StreamVoices)
		{
			if (ptr->m_nVoiceId == id)
			{
				// Stream found. Adding to deleting queue.

				{
					std::unique_lock qlock{ m_DeleteStreamLock };

					m_DeleteStreamQueue.push(ptr.get());
				}

				m_cvDeleteStream.notify_one();

				return;
			}
		}
	}
}

void XAudioPlayer::ThreadCreator()
{
	XAudioPlayer& p = *this;

	std::unique_lock lock{ p.m_CreateLock };

	for (;;)
	{
		p.m_cvCreate.wait(lock, [&p]() { return PredCreator(p); });

		if (m_bTerminateFlag)
		{
			// Unloading.

			break;
		}

		// Queue is not empty. Create voices.
		while (!m_CreateQueue.empty())
		{
			std::shared_ptr<XPlainVoice> spVoice = m_CreateQueue.front();

			// Voice processing. 
			
			try
			{
				spVoice->Init();
				spVoice->FetchData();
				spVoice->SubmitAndPlay();

				{
					std::unique_lock vlock{ m_VoicesLock };

					// Make sure that current count of playing voices is lesser than max.
					m_Voices.resize(GetMaxVoices() - 1);

					// Add new voice to the top.
					m_Voices.push_front(spVoice);
				}

			}
			catch(const XException& e)
			{
				// Something bad happened.

				PostNotify(XNotifyMessage(spVoice->m_nVoiceId, XAudioPlayer::XPlayerNotify::XN_ERROR,
					std::make_any<XException>(e)));
			}

			spVoice = nullptr;

			m_CreateQueue.pop();
		}
	}
}

bool PredDeleter(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_DeleteQueue.size());
}

void XAudioPlayer::ThreadDeleter()
{
	XAudioPlayer& p = *this;

	std::unique_lock lock{ p.m_DeleteLock };

	for (;;)
	{
		p.m_cvDelete.wait(lock, [&p]() { return PredDeleter(p); });

		if (m_bTerminateFlag)
		{
			// Unloading.

			break;
		}

		// Queue is not empty. Delete voices.
		while (!m_DeleteQueue.empty())
		{
			XPlainVoice* pVoice = m_DeleteQueue.front();

			// Voice processing.
			{
				std::unique_lock vlock{ m_VoicesLock };

				for (auto it = m_Voices.begin(); it != m_Voices.end(); ++it)
				{
					if (it->get() == pVoice)
					{
						// Remove this voice.

						m_Voices.erase(it);
						break;
					}
				}
			}

			m_DeleteQueue.pop();
		}
	}
}

bool PredStreamCreator(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_CreateStreamQueue.size());
}

void XAudioPlayer::ThreadStreamCreator()
{
	XAudioPlayer& p = *this;

	std::unique_lock lock{ p.m_CreateStreamLock };

	for (;;)
	{
		p.m_cvCreateStream.wait(lock, [&p]() { return PredStreamCreator(p); });

		if (m_bTerminateFlag)
		{
			// Unloading.

			break;
		}

		// Queue is not empty. Create voices.
		while (!m_CreateStreamQueue.empty())
		{
			std::shared_ptr<XStreamVoice> spStreamVoice = m_CreateStreamQueue.front();

			// Voice processing. 

			try
			{
				spStreamVoice->CreateAndPlay();
			}
			catch (const XException& e)
			{
				// Something bad happened.

				PostNotify(XNotifyMessage(spStreamVoice->m_nVoiceId, XAudioPlayer::XPlayerNotify::XN_ERROR,
					std::make_any<XException>(e)));
			}

			{
				std::unique_lock vslock{ m_StreamVoicesLock };

				// Add new stream voice to the top.
				m_StreamVoices.push_front(spStreamVoice);
			}

			spStreamVoice = nullptr;

			m_CreateStreamQueue.pop();
		}
	}
}

bool PredStreamDeleter(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_DeleteStreamQueue.size());
}

void XAudioPlayer::ThreadStreamDeleter()
{
	XAudioPlayer& p = *this;

	std::unique_lock lock{ p.m_DeleteStreamLock };

	for (;;)
	{
		p.m_cvDeleteStream.wait(lock, [&p]() { return PredStreamDeleter(p); });

		if (m_bTerminateFlag)
		{
			// Unloading.

			break;
		}

		// Queue is not empty. Delete voices.
		while (!m_DeleteStreamQueue.empty())
		{
			XStreamVoice* pStreamVoice = m_DeleteStreamQueue.front();

			// Voice processing.
			{
				std::unique_lock vslock{ m_StreamVoicesLock };

				for (auto it = m_StreamVoices.begin(); it != m_StreamVoices.end(); ++it)
				{
					if (it->get() == pStreamVoice)
					{
						// Remove this voice.

						m_StreamVoices.erase(it);
						break;
					}
				}
			}

			m_DeleteStreamQueue.pop();
		}
	}
}

bool PredNotifier(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_NotifyQueue.size());
}

void XAudioPlayer::ThreadNotifier()
{
	XAudioPlayer& p = *this;

	std::unique_lock lock{ p.m_NotifyLock };

	for (;;)
	{
		p.m_cvNotify.wait(lock, [&p]() { return PredNotifier(p); });

		if (m_bTerminateFlag)
		{
			// Unloading.

			break;
		}

		if (!m_pCallback)
		{
			// Callback pointer is not set. Clearing queue and leaving.
			XAux::ClearAdapter(m_NotifyQueue);
			continue;
		}

		// Queue is not empty. Processing callbacks.
		while (!m_NotifyQueue.empty())
		{
			auto msg = m_NotifyQueue.front();

			switch (msg.m_nMessageId)
			{
			    case XPlayerNotify::XN_STREAMING_STOP:
			    {
    				// Streaming ended.
					m_pCallback->OnStreamingEnd(msg.m_nVoiceId);

				    break;
			    }
				case XPlayerNotify::XN_ERROR:
				{
					// Error.
					m_pCallback->OnError(msg.m_nVoiceId, std::any_cast<XException>(msg.m_Extra));

					break;
				}
				default:
			    {
    				break;
	    		}
			}

			m_NotifyQueue.pop();
		}
	}
}

void XAudioPlayer::PostNotify(XNotifyMessage msg)
{
	{
		std::unique_lock qlock{ m_NotifyLock };

		m_NotifyQueue.push(msg);
	}

	m_cvNotify.notify_one();
}

