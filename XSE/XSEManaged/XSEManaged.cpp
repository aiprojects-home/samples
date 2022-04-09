#include "pch.h"

#include "XSEManaged.h"
#include "Vcclr.h"

namespace XSEManaged
{
	void XSoundEngine::Init(XSEInitParams params)
	{
		if (!m_bInit)
		{
			m_PlaybackCompleteDelegate = gcnew _PlaybackCompleteDelegate(&XSoundEngine::OnPlaybackComplete);
			m_ErrorDelegate = gcnew _ErrorDelegate(&XSoundEngine::OnError);

			m_hPlayback = GCHandle::Alloc(m_PlaybackCompleteDelegate);
			m_hError = GCHandle::Alloc(m_ErrorDelegate);

			IntPtr PlaybackPtr = Marshal::GetFunctionPointerForDelegate(m_PlaybackCompleteDelegate);
			IntPtr ErrorPtr = Marshal::GetFunctionPointerForDelegate(m_ErrorDelegate);

			pin_ptr<const wchar_t> pStorageFile = PtrToStringChars(params.strStorageFile);
			pin_ptr<const wchar_t> pBankFile = PtrToStringChars(params.strBankFile);

			XSE_INIT p;

			p.pOnPlaybackComplete = static_cast<XSE_ON_PLAYBACK_COMPLETE_CALLBACK*>(PlaybackPtr.ToPointer());
			p.pOnError = static_cast<XSE_ON_ERROR_CALLBACK*>(ErrorPtr.ToPointer());
			p.pStorageFile =  pStorageFile;
			p.pBankFile = pBankFile;
			p.bExtendedMode = params.bExtendedMode;
			p.nMaxBankSize = params.nMaxBankSize;
			p.nMaxVoices = params.nMaxVoices;
			p.nMaxStreams = params.nMaxStreams;

			if (!XSEInit(p))
			{
				m_bInit = true;
				return;
			}
			else
			{
				m_hError.Free();
				m_hPlayback.Free();

				// Ошибка во время инициализации.
				ThrowLastError();
			}
		};
	}

	void XSoundEngine::Done()
	{
		if ( (m_bInit) && (!XSEDone()))
		{
			m_bInit = false;
			m_hError.Free();
			m_hPlayback.Free();

			return;
		}

		// Нативный код всегда возвращает 0.
	}

	void XSoundEngine::Play(uint16_t nId, float fVolume, float fPan)
	{
		if (XSEPlaySimple(nId, fVolume, fPan))
		{
			ThrowLastError();
		}
	}
	
	void XSoundEngine::PlayStream(uint16_t nId, float fVolume)
	{
		if (XSEPlayStream(nId, fVolume))
		{
			ThrowLastError();
		}
	}

	void XSoundEngine::StopStream(uint16_t nId)
	{
		if (XSEStopStream(nId))
		{
			ThrowLastError();
		}
	}

	void XSoundEngine::StopAll()
	{
		if (XSEStopAll())
		{
			ThrowLastError();
		}
	}

	void XSoundEngine::Suspend()
	{
		if (XSESuspend())
		{
			ThrowLastError();
		}
	}

	void XSoundEngine::Resume()
	{
		if (XSEResume())
		{
			ThrowLastError();
		}
	}

	void XSoundEngine::CreateStorage(System::String^ strDir, System::String^ strStorage)
	{
		pin_ptr<const wchar_t> pStorageFile = PtrToStringChars(strStorage);
		pin_ptr<const wchar_t> pDirectory = PtrToStringChars(strDir);

		if (XSECreateStorage(pDirectory, pStorageFile))
		{
			ThrowLastError();
		}
	}

	bool XSoundEngine::COMInitializer(bool bInit)
	{
		int nResult = XSECOMInitializer(bInit);

		return (bool)(!nResult);
	}

	void XSoundEngine::OnError(uint16_t nId, wchar_t* pErrorInfo)
	{
		OnErrorEvent(nId, gcnew System::String(pErrorInfo));
	};

	void XSoundEngine::OnPlaybackComplete(uint16_t nId)
	{
		OnPlaybackCompleteEvent(nId);
	}

	void XSoundEngine::ThrowLastError()
	{
		wchar_t strErrorInfo[4096];

		XSEGetLastErrorInfo(strErrorInfo, 4096);

		throw gcnew System::Exception(gcnew System::String(strErrorInfo));
	}

}
