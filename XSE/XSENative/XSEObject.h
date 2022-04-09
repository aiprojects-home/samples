#pragma once

#include "XGlobals.h"
#include "IXAudioPlayerCallback.h"

// �����, ����������� ��� ���������� �������.

class XAudioPlayer;
class XSoundBank;

// ��������� ��� ������� �������.
using XSE_ON_ERROR_CALLBACK = void (uint16_t, const wchar_t*);
using XSE_ON_PLAYBACK_COMPLETE_CALLBACK = void  (uint16_t);

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

class XSEObject : public IXAudioPlayerCallback
{
private:

	static std::unique_ptr<XSEObject> m_upCurrent;     // ��������� �� ����������� ���������
	static std::mutex                 m_mtxStaticLock; // �������� ��� ����������
	mutable std::shared_mutex         m_mtxLock;

	std::wstring                      m_strLastError;   // ���������� � ��������� ������
	bool                              m_bInit;          // TRUE, ���� ������������� �������

	std::unique_ptr<XAudioPlayer>     m_upAudioPlayer;  // ���������� ����������: ������������� � ��� ����
	std::unique_ptr<XSoundBank>       m_upSoundBank;

	XSE_ON_ERROR_CALLBACK             *m_pErorrCallback;
	XSE_ON_PLAYBACK_COMPLETE_CALLBACK *m_pPlaybackCompleteCallback;

private:
	XSEObject();

public:
	static XSEObject& Current();

	// ������ ��� ������ �� ��������� DLL.
	
	int CreateStorage(const wchar_t* pDir, const wchar_t* pStorage);

	const wchar_t* GetLastError() const;

	int GetLastErrorInfo(wchar_t *pBuffer, int nSize);

	int Init(const XSE_INIT params);

	int Done();

	int Suspend();

	int Resume();

	int PlaySimple(uint16_t nId, float fVolume = 1.0f, float fPan = 0.0f);

	int PlayStream(uint16_t nId, float fVolume = 1.0f);

	int StopStream(uint16_t nId);

	int StopAll();

private:

	// ���������� callback.

	void OnStreamingEnd(uint16_t id);
	void OnError(const uint16_t id, const XException e);
};

