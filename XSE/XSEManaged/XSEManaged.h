#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

#include <string>

// ��������� ��� ������� �������.
using XSE_ON_ERROR_CALLBACK = void(uint16_t, const wchar_t*);
using XSE_ON_PLAYBACK_COMPLETE_CALLBACK = void(uint16_t);

// ��������� ��� �������������.
using XSE_INIT = struct
{
	const wchar_t* pStorageFile;  // ��� ����� - ����������
	const wchar_t* pBankFile;     // ��� ����� �����
	bool           bExtendedMode; // ���� ������������ ������
	uint8_t        nMaxVoices;    // ����. ���������� �������
	uint8_t        nMaxStreams;   // ����. ���������� �������
	uint32_t       nMaxBankSize;  // ����. ������ �����

	XSE_ON_ERROR_CALLBACK*             pOnError;
	XSE_ON_PLAYBACK_COMPLETE_CALLBACK* pOnPlaybackComplete;
};

_declspec(dllimport) int XSECreateStorage(const wchar_t* pDir, const wchar_t* pStorage);
_declspec(dllimport) wchar_t* XSEGetLastError();
_declspec(dllimport) int XSEGetLastErrorInfo(wchar_t* pBuffer, int nSize);
_declspec(dllimport) int XSEInit(const XSE_INIT params);
_declspec(dllimport) int XSEDone();
_declspec(dllimport) int XSESuspend();
_declspec(dllimport) int XSEResume();
_declspec(dllimport) int XSEPlaySimple(uint16_t nId, float fVolume = 1.0f, float fPan = 0.0f);
_declspec(dllimport) int XSEPlayStream(uint16_t nId, float fVolume = 1.0f);
_declspec(dllimport) int XSEStopStream(uint16_t nId);
_declspec(dllimport) int XSEStopAll();
_declspec(dllimport) int XSECOMInitializer(bool bInit);

namespace XSEManaged 
{
	// ��������� ��� ������������� ������.
	public value struct XSEInitParams
	{
		System::String^ strStorageFile;
		System::String^ strBankFile;
		bool            bExtendedMode;
		int             nMaxVoices;
		int             nMaxStreams;
		int             nMaxBankSize;
	};

	// ��������� ��� ���������.

	public delegate void PlaybackCompleteDelegate(uint16_t nId);
	public delegate void ErrorDelegate(uint16_t nId, System::String^ strErrorInfo);

	private delegate void _PlaybackCompleteDelegate(uint16_t nId);
	private delegate void _ErrorDelegate(uint16_t, wchar_t *pErrorInfo);

		// ����� ������, ��������� �� .NET.
	public ref class XSoundEngine abstract sealed
	{
	public:
		
		static event PlaybackCompleteDelegate^ OnPlaybackCompleteEvent;
		static event ErrorDelegate^            OnErrorEvent;

		static void Init(XSEInitParams params);
		static void Done();

		static void Play(uint16_t nId, float fVolume, float fPan);
		static void PlayStream(uint16_t nId, float fVolume);
		static void StopStream(uint16_t nId);
		static void StopAll();

		static void Suspend();
		static void Resume();

		static void CreateStorage(System::String^ strDir, System::String^ strStorage);

		static bool COMInitializer(bool bInit);

	private:

		static bool m_bInit = false;

		// ���������� ����������� �������� �������.

		static _PlaybackCompleteDelegate^ m_PlaybackCompleteDelegate;
		static _ErrorDelegate^            m_ErrorDelegate;
		static GCHandle                   m_hPlayback;
		static GCHandle                   m_hError;

		static void OnError(uint16_t nId, wchar_t* pErrorInfo);
		static void OnPlaybackComplete(uint16_t nId);

		static void ThrowLastError();
	};
}
