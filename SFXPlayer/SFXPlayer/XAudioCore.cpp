#include "XAudioCore.h"
#include "XException.h"

std::unique_ptr<XAudioCore> XAudioCore::m_spCurrent{ nullptr };

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
	if (m_spCurrent == nullptr)
	{
		m_spCurrent.reset(new XAudioCore());
	}

	return *(m_spCurrent.get());
}

void XAudioCore::Init()
{
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
	return m_pXAudio2;
}

XAudioCore::operator IXAudio2MasteringVoice*()
{
	return m_pXAudio2MV;
}

