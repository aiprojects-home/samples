#include "pch.h"
#include "XSEObject.h"
#include "XSoundBank.h"
#include "XAudioPlayer.h"
#include "XAudioCore.h"
#include "XEFM.h"

std::unique_ptr<XSEObject> XSEObject::m_upCurrent;
std::mutex                 XSEObject::m_mtxStaticLock;

const wchar_t *pNotInitializedError = L"Not initialized";
const wchar_t *pCriticalFault = L"Critical fault";

XSEObject& XSEObject::Current()
{
	std::unique_lock lock{ m_mtxStaticLock };

	if (m_upCurrent == nullptr)
	{
		m_upCurrent.reset(new XSEObject{});
	}

	return *(m_upCurrent.get());
}

XSEObject::XSEObject()
{
	m_bInit = false;

	m_pErorrCallback = nullptr;
	m_pPlaybackCompleteCallback = nullptr;
}

int XSEObject::CreateStorage(const wchar_t* pDir, const wchar_t* pStorage)
{
	std::unique_lock lock{ m_mtxLock };

	try
	{
		XEFM::Current().CreateStorage(pDir, pStorage);
	}
	catch (XException& e)
	{
		m_strLastError = e.GetDumpInfo();

		return -1;
	}

	return 0;
}

const wchar_t* XSEObject::GetLastError() const
{
	std::shared_lock lock{ m_mtxLock };

	return m_strLastError.c_str();
}

int XSEObject::GetLastErrorInfo(wchar_t *pBuffer, int nSize)
{
	std::shared_lock lock{ m_mtxLock };

	size_t nCharsToCopy = min(nSize - 1, m_strLastError.size());

	std::fill(pBuffer, pBuffer + nSize, 0);
	std::copy(m_strLastError.begin(), m_strLastError.begin() + nCharsToCopy, pBuffer);

	return 0;
}


int XSEObject::Init(XSE_INIT params)
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		// Уже инициализирован.
		return 0;
	}

	try
	{
		// Инициализируем XAudio2
		XAudioCore::Current().Init();

		// Открываем контейнер
		XEFM::Current().AssignFile(params.pStorageFile);
		XEFM::Current().SetExtendedMode(params.bExtendedMode);

		// Создаем банк и загружаем его
		m_upSoundBank = std::make_unique<XSoundBank>(params.nMaxBankSize);
		m_upSoundBank->AssignFile(params.pBankFile);

		// Создаем проигрыватель
		m_upAudioPlayer = std::make_unique<XAudioPlayer>();
		m_upAudioPlayer->SetCallback(this);
		m_upAudioPlayer->SetMaxVoices(params.nMaxVoices);
		m_upAudioPlayer->SetMaxStreamVoices(params.nMaxStreams);
		m_upAudioPlayer->AttachSoundBank(m_upSoundBank.get());

		m_upAudioPlayer->Start();

		m_pErorrCallback = params.pOnError;
		m_pPlaybackCompleteCallback = params.pOnPlaybackComplete;
	}
	catch (XException& e)
	{

		m_upAudioPlayer.reset();
		m_upSoundBank.reset();

		XEFM::Current().Reset();
		XAudioCore::Current().Done();

		m_strLastError = e.GetDumpInfo();

		return -1;
	}

	m_bInit = true;

	return 0;
}

int XSEObject::Done()
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		m_upAudioPlayer.reset();
		m_upSoundBank.reset();

		XEFM::Current().Reset();
		XAudioCore::Current().Done();

		m_bInit = false;
	}

	return 0;
}

int XSEObject::Suspend()
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		m_upAudioPlayer->Stop();

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

int XSEObject::Resume()
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		m_upAudioPlayer->Start();

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

int XSEObject::PlaySimple(uint16_t nId, float fVolume, float fPan)
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		try
		{
			m_upAudioPlayer->PlaySimple(nId, fVolume, fPan);
		}
		catch (XException& e)
		{
			m_strLastError = e.GetDumpInfo();
			return -1;
		}

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

int XSEObject::PlayStream(uint16_t nId, float fVolume)
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		try
		{
			m_upAudioPlayer->PlayStream(nId, fVolume);
		}
		catch (XException& e)
		{
			m_strLastError = e.GetDumpInfo();
			return -1;
		}

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

int XSEObject::StopStream(uint16_t nId)
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		m_upAudioPlayer->StopStream(nId);

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

int XSEObject::StopAll()
{
	std::unique_lock lock{ m_mtxLock };

	if (m_bInit)
	{
		m_upAudioPlayer->StopAll();

		return 0;
	}

	m_strLastError = pNotInitializedError;

	return -1;
}

void XSEObject::OnStreamingEnd(uint16_t id)
{
	if (m_pPlaybackCompleteCallback)
	{
		m_pPlaybackCompleteCallback(id);
	}
}

void XSEObject::OnError(const uint16_t id, const XException e)
{
	if (m_pErorrCallback)
	{
		m_pErorrCallback(id, e.GetDumpInfo().c_str());
	}
}
