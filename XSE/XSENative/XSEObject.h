#pragma once

#include "XGlobals.h"
#include "IXAudioPlayerCallback.h"

// Класс, связывающий все компоненты воедино.

class XAudioPlayer;
class XSoundBank;

// Сигнатура для внешних вызовов.
using XSE_ON_ERROR_CALLBACK = void (uint16_t, const wchar_t*);
using XSE_ON_PLAYBACK_COMPLETE_CALLBACK = void  (uint16_t);

// Структура для инициализации.
using XSE_INIT = struct
{
	const wchar_t* pStorageFile;  // имя файла - контейнера
	const wchar_t* pBankFile;     // имя файла банка
	bool           bExtendedMode; // флаг расширенного режима
	uint8_t        nMaxVoices;    // макс. количество голосов
	uint8_t        nMaxStreams;   // макс. количество потоков
	uint32_t       nMaxBankSize;  // макс. размер банка

	XSE_ON_ERROR_CALLBACK*             pOnError;
	XSE_ON_PLAYBACK_COMPLETE_CALLBACK* pOnPlaybackComplete;
};

class XSEObject : public IXAudioPlayerCallback
{
private:

	static std::unique_ptr<XSEObject> m_upCurrent;     // указатель на едиственный экземпляр
	static std::mutex                 m_mtxStaticLock; // мьютексы для блокировки
	mutable std::shared_mutex         m_mtxLock;

	std::wstring                      m_strLastError;   // информация о последней ошибке
	bool                              m_bInit;          // TRUE, если инициализация успешна

	std::unique_ptr<XAudioPlayer>     m_upAudioPlayer;  // внутренние компоненты: проигрыватель и его банк
	std::unique_ptr<XSoundBank>       m_upSoundBank;

	XSE_ON_ERROR_CALLBACK             *m_pErorrCallback;
	XSE_ON_PLAYBACK_COMPLETE_CALLBACK *m_pPlaybackCompleteCallback;

private:
	XSEObject();

public:
	static XSEObject& Current();

	// Методы для вызова их функциями DLL.
	
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

	// Внутренний callback.

	void OnStreamingEnd(uint16_t id);
	void OnError(const uint16_t id, const XException e);
};

