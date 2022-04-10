#include "pch.h"
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
}

XAudioPlayer::~XAudioPlayer()
{
	Stop();
}

void XAudioPlayer::AttachSoundBank(XSoundBank* pBank)
{
	std::unique_lock lock{ m_MainLock };

	if (m_bRunning)
	{
		throw XException(L"XAudioPlayer::AttachSoundBank(): can't attach bank while running");
	}

	m_pSoundBank = pBank;
}

void XAudioPlayer::SetMaxVoices(const uint8_t count)
{
	m_nMaxVoices = count;
}

uint8_t XAudioPlayer::GetMaxVoices() const
{
	return m_nMaxVoices;
}

void XAudioPlayer::SetMaxStreamVoices(const uint8_t count)
{
	m_nMaxStreamVoices = count;
}

uint8_t XAudioPlayer::GetMaxStreamVoices() const
{
	return m_nMaxStreamVoices;
}

void XAudioPlayer::SetCallback(IXAudioPlayerCallback *pCallback)
{
	m_pCallback = pCallback;
}

void XAudioPlayer::Start()
{
	std::unique_lock lock{ m_MainLock };

	if (m_bRunning)
	{
		// Уже работает.
		return;
	}

	if (m_pSoundBank == nullptr)
	{
		throw XException(L"XAudioPlayer::Start(): soundbank is not attached");
	}

	// Очищаем флаги и запускаем потоки.
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
		// Уже остановлен.
		return;
	}

	// Устанавливаем флаг завершения и ждем, пока потоки не завершатся.
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

	// Чистим данные.

	// 1. Очереди
	XAux::ClearAdapter(m_CreateQueue);
	XAux::ClearAdapter(m_CreateStreamQueue);
	XAux::ClearAdapter(m_DeleteQueue);
	XAux::ClearAdapter(m_DeleteStreamQueue);
	XAux::ClearAdapter(m_NotifyQueue);

	// 2. Голоса на воспроизведении

	m_Voices.clear();
	m_StreamVoices.clear();

	// 3. Пул голосов

	m_VoicePool.Clear();

	m_bRunning = false;
}

void XAudioPlayer::PlaySimple(const uint16_t nId, float fVolume, float fPan)
{
	std::unique_lock lock{ m_MainLock };

	if (!m_bRunning)
	{
		throw XException(L"XAudioPlayer::PlaySimple(): player is not running");
	}

	if (!m_pSoundBank->CanFetch(nId))
	{
		throw XException(L"XAudioPlayer::PlaySimple(): can't fetch sound with id='%d'", nId);
	}

	// Все в порядке. Добавляем в очередь на создание.

	{
		std::unique_lock qlock{ m_CreateLock };

		std::shared_ptr<XAudioVoice> spVoice = std::make_shared<XAudioVoice>();
		spVoice->SetPan(fPan);
		spVoice->SetVolume(fVolume);

		spVoice->Init(nId, this, &m_VoicePool, m_pSoundBank);

		m_CreateQueue.push(spVoice);

	}

	m_cvCreate.notify_one();
}

void XAudioPlayer::PlayStream(const uint16_t nId, float fVolume)
{
	std::unique_lock lock{ m_MainLock };

	if (!m_bRunning)
	{
		throw XException(L"XAudioPlayer::PlayStream(): player is not running");
	}

	// Проверяем сколько потоков уже воспроизводятся.

	std::unique_lock vlock{ m_StreamVoicesLock };
	{
		if (m_StreamVoices.size() >= m_nMaxStreamVoices)
		{
			// Больше уже нельзя.
			return;
		}
	}

	if (!m_pSoundBank->CanStream(nId))
	{
		throw XException(L"XAudioPlayer::PlayStream(): can't stream music with id='%d'", nId);
	}

	// Все OK. Добавляем поток в очередь на создание.

	{
		std::unique_lock qlock{ m_CreateStreamLock };

		std::shared_ptr<XAudioVoice> spVoice = std::make_shared<XAudioVoice>();
		spVoice->SetVolume(fVolume);

		spVoice->Init(nId, this, &m_VoicePool, m_pSoundBank);

		m_CreateStreamQueue.push(spVoice);
	}

	m_cvCreateStream.notify_one();
}

void XAudioPlayer::StopStream(const uint16_t id)
{
	std::unique_lock lock{ m_MainLock };

	{
		// Ищем поток с указанным id.

		std::unique_lock vlock{ m_StreamVoicesLock };
		
		for (auto &ptr : m_StreamVoices)
		{
			if (ptr->GetId() == id)
			{
				// Нашли, добавляем его в очередь на удаление.

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

void XAudioPlayer::StopAll()
{
	std::unique_lock lock1{ m_MainLock };
	std::unique_lock lock2{ m_VoicesLock };
	std::unique_lock lock3{ m_StreamVoicesLock };

	m_Voices.clear();
	m_StreamVoices.clear();
}

void XAudioPlayer::OnPlaybackComplete(XAudioVoice* pVoice, bool bStream)
{
	if (bStream)
	{
		// Закончилось потоковое восрпоизведение. Добавляем в очередь на удаление и отправляем уведомление.
		{
			std::unique_lock qlock{ m_DeleteStreamLock };

			m_DeleteStreamQueue.push(pVoice);
		}

		m_cvDeleteStream.notify_one();

		PostNotify(XAudioPlayer::XNotifyMessage(pVoice->GetId(), XAudioPlayer::XPlayerNotify::XN_STREAMING_STOP));

	}
	else
	{
		// Закончилось обычное воспроизведение. Добавляем в очередь на удаление.
		{
			std::unique_lock lock{ m_DeleteLock };

			m_DeleteQueue.push(pVoice);
		}

		m_cvDelete.notify_one();
	}
}

bool PredCreator(XAudioPlayer& pObject)
{
	return (pObject.m_bTerminateFlag || pObject.m_CreateQueue.size());
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
			// Выгружаемся.

			break;
		}

		// В очереди что-то есть. Создаем голоса.

		while (!m_CreateQueue.empty())
		{
			std::shared_ptr<XAudioVoice> spVoice = m_CreateQueue.front();

			// Обработка голоса. 
			
			try
			{
				spVoice->Play(false);

				{
					std::unique_lock vlock{ m_VoicesLock };

					// Нужно гарантировать, что количество воспроизводимых голосов меньше максимума.
					m_Voices.resize(GetMaxVoices() - 1);

					// Добавляем новый голос в начало.
					m_Voices.push_front(spVoice);
				}

			}
			catch(const XException& e)
			{
				// Что-то случилось, отправляем уведомление с ошибкой.

				PostNotify(XNotifyMessage(spVoice->GetId(), XAudioPlayer::XPlayerNotify::XN_ERROR,
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
			// Выгружаемся.

			break;
		}

		// В очереди что-то есть, удаляем голоса.
		while (!m_DeleteQueue.empty())
		{
			XAudioVoice* pVoice = m_DeleteQueue.front();

			// Обработка голоса.
			{
				std::unique_lock vlock{ m_VoicesLock };

				for (auto it = m_Voices.begin(); it != m_Voices.end(); ++it)
				{
					if (it->get() == pVoice)
					{
						// Удаляем голос.

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
			// Выгружаемся.

			break;
		}

		// В очереди что-то есть, создаем потоковые голоса.
		while (!m_CreateStreamQueue.empty())
		{
			std::shared_ptr<XAudioVoice> spStreamVoice = m_CreateStreamQueue.front();

			// Обработка потокового голоса. 

			try
			{
				spStreamVoice->Play(true);
			}
			catch (const XException& e)
			{
				// Что-то случилось, отправляем уведомление с ошибкой.

				PostNotify(XNotifyMessage(spStreamVoice->GetId(), XAudioPlayer::XPlayerNotify::XN_ERROR,
					std::make_any<XException>(e)));
			}

			{
				std::unique_lock vslock{ m_StreamVoicesLock };

				// Добавляем новый голос в начало.
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
			// Выгружаемся.

			break;
		}

		// В очереди что-то есть, удаляем голоса.
		while (!m_DeleteStreamQueue.empty())
		{
			XAudioVoice* pStreamVoice = m_DeleteStreamQueue.front();

			// Обработка голосов.
			{
				std::unique_lock vslock{ m_StreamVoicesLock };

				for (auto it = m_StreamVoices.begin(); it != m_StreamVoices.end(); ++it)
				{
					if (it->get() == pStreamVoice)
					{
						// Удаляем этот голос.

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
			// Выгружаемся.

			break;
		}

		if (!m_pCallback)
		{
			// Указатель не был установлен, чистим очередь и выходим.
			XAux::ClearAdapter(m_NotifyQueue);
			continue;
		}

		// В очереди что-то есть, обрабатываем сообщения.
		while (!m_NotifyQueue.empty())
		{
			auto msg = m_NotifyQueue.front();

			switch (msg.m_nMessageId)
			{
			    case XPlayerNotify::XN_STREAMING_STOP:
			    {
    				// Потоковое воспроизведение закончилось.
					m_pCallback.load()->OnStreamingEnd(msg.m_nVoiceId);

				    break;
			    }
				case XPlayerNotify::XN_ERROR:
				{
					// Некритическая ошибка во время воспроизведения.
					m_pCallback.load()->OnError(msg.m_nVoiceId, std::any_cast<XException>(msg.m_Extra));

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

