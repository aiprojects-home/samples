#include "XVoiceObject.h"
#include "XException.h"
#include "XAudioCore.h"
#include "XAudioVoice.h"

XVoiceObject::XVoiceObject(const WAVEFORMATEX& wfex)
{
	m_Format = wfex;
	m_pExternalCallback = nullptr;

	HRESULT hr;
	IXAudio2* pXAudio = (IXAudio2*)XAudioCore::Current();

	// Try to create source voice.
	if (FAILED(hr = pXAudio->CreateSourceVoice(&m_pVoice, &m_Format, 0, 2.0f, this)))
	{
		throw XException(L"XVoiceObject::XVoiceObject(): can't create voice");
	}

}

XVoiceObject::~XVoiceObject()
{
	if (m_pVoice)
	{
		m_pVoice->DestroyVoice();
	}
}

XVoiceObject::operator IXAudio2SourceVoice*()
{
	return m_pVoice;
}

void XVoiceObject::SetCallback(IXAudio2VoiceCallback* pCallback)
{
	m_pExternalCallback = pCallback;
}

void XVoiceObject::Reset()
{
	m_pExternalCallback = nullptr;
	if (m_pVoice)
	{
		m_pVoice->Stop();
		m_pVoice->FlushSourceBuffers();
	}
}

bool XVoiceObject::IsEqual(const WAVEFORMATEX& wfex)
{
	return ((m_Format.wFormatTag == wfex.wFormatTag) &&
		    (m_Format.nChannels == wfex.nChannels) &&
		    (m_Format.wBitsPerSample == wfex.wBitsPerSample) && 
		    (m_Format.nSamplesPerSec == wfex.nSamplesPerSec));
}

void XVoiceObject::OnStreamEnd()
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnStreamEnd();
	}
}

void XVoiceObject::OnVoiceProcessingPassEnd()
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnVoiceProcessingPassEnd();
	}
}

void XVoiceObject::OnVoiceProcessingPassStart(UINT32 SamplesRequired)
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnVoiceProcessingPassStart(SamplesRequired);
	}
}

void XVoiceObject::OnBufferEnd(void * pBufferContext)
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnBufferEnd(pBufferContext);
	}
}

void XVoiceObject::OnBufferStart(void * pBufferContext)
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnBufferStart(pBufferContext);
	}
}

void XVoiceObject::OnLoopEnd(void * pBufferContext)
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnLoopEnd(pBufferContext);
	}
}

void XVoiceObject::OnVoiceError(void * pBufferContext, HRESULT Error)
{
	if (m_pExternalCallback)
	{
		m_pExternalCallback->OnVoiceError(pBufferContext, Error);
	}
}
