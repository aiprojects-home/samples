#pragma once

#include "XSoundDecoder.h"
#include <shared_mutex>
#include "XAux.h"

class XPCMDecoder :	public XSoundDecoder
{
private:
	std::wstring    m_strFileName; // имя файла
	uint32_t        m_nFileSize;   // размер данных (байт)
	WAVEFORMATEX    m_SoundFormat; // формат файла

	bool            m_bLoaded;     // TRUE, файл загружен в память
	bool            m_bAssigned;   // TRUE, файл открыт
	bool            m_bDecoding;   // TRUE, файл декодируется
	uint32_t        m_nRefCount;   // счетчик ссылок

	// Данные файла (звуковые сэмплы, распакованы):
	std::unique_ptr<BYTE[]> m_upSamples;

	// Данные для потокового декодирования:
	uint32_t m_nBytesRemain;
	uint32_t m_nCurrentOffset;
	uint32_t m_nDataOffset;

	// Мьютекс для потокобезопасности:
	mutable std::shared_mutex m_mtxMain;

public:
	XPCMDecoder();
	virtual ~XPCMDecoder();

	static XSoundDecoder* CreateStatic();

private:

	void _OpenFile(const wchar_t* pFileName, bool bLoad = false);

	void _ResetInternalData();

public:

	// Реализация интерфейса XSoundDecoder.

	virtual void AssignFile(const wchar_t* pFileName);
	virtual bool ReleaseFile();
	virtual bool IsAssigned();
	virtual bool Load();
	virtual bool Unload();
	virtual bool IsLoaded();
	virtual uint32_t GetSize();
	virtual bool GetFormat(WAVEFORMATEX & refFormat);
	virtual bool GetData(std::unique_ptr<BYTE[]> &refData);
	virtual bool GetDataDirect(BYTE*& refData);
	virtual void FreeData();
	virtual void DecodeStart(WAVEFORMATEX &wfex);
	virtual uint32_t DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount);
	virtual bool DecodeStop();
	virtual bool IsDecoding();
};

