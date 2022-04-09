#include "pch.h"
#include "XAudioCore.h"
#include "XException.h"

std::mutex                  XAudioCore::m_StaticLock;
std::unique_ptr<XAudioCore> XAudioCore::m_upCurrent{ nullptr };

XAudioCore::XAudioCore()
{
	m_pXAudio2 = nullptr;
	m_pXAudio2MV = nullptr;
}

XAudioCore::~XAudioCore()
{
}

XAudioCore& XAudioCore::Current()
{
	std::unique_lock lock{ m_StaticLock };

	if (m_upCurrent == nullptr)
	{
		m_upCurrent.reset(new XAudioCore());
	}

	return *(m_upCurrent.get());
}

void XAudioCore::Init()
{
	std::unique_lock lock{ m_MainLock };

	HRESULT hr;

	if (FAILED(hr = XAudio2Create(&m_pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		throw XException(L"XAudioCore::Init(): XAudio2Create() error");
	}

	if (FAILED(hr = m_pXAudio2->CreateMasteringVoice(&m_pXAudio2MV)))
	{
		throw XException(L"XAudioCore::Init(): CreateMasteringVoice() error");
	}
}

void XAudioCore::Done()
{
	std::unique_lock lock{ m_MainLock };

	if (m_pXAudio2MV)
	{
		m_pXAudio2MV->DestroyVoice();
		m_pXAudio2MV = nullptr;
	};

	if (m_pXAudio2)
	{
		m_pXAudio2->Release();
		m_pXAudio2 = nullptr;
	};
};

XAudioCore::operator IXAudio2*()
{
	std::shared_lock lock{ m_MainLock };

	return m_pXAudio2;
}

XAudioCore::operator IXAudio2MasteringVoice*()
{
	std::shared_lock lock{ m_MainLock };

	return m_pXAudio2MV;
}

