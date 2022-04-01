#pragma once

#include "XSoundDecoder.h"
#include <wmsdk.h>
#include <shared_mutex>
#include "XAux.h"

class XWMADecoder :	public XSoundDecoder
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
	std::unique_ptr<IWMSyncReader, XAux::COM_deleter_type> m_upWMReader;
	std::unique_ptr<INSSBuffer, XAux::COM_deleter_type>    m_upBuffer;
	std::unique_ptr<IStream, XAux::COM_deleter_type>       m_upStream;

	uint32_t m_nBytesRemain;
	uint32_t m_nRemainOffset;

	// Мьютекс для потокобезопасности:
	mutable std::shared_mutex m_mtxMain;

public:
	XWMADecoder();
	virtual ~XWMADecoder();

	static XSoundDecoder* CreateStatic();

private:
	
	// Режимы загрузки файла для метода _OpenFile()
	enum class OpenMode : uint8_t
	{
		OPEN_ANALYSE = 0,      // определение формата
		OPEN_LOAD_SAMPLES = 1, // определение формата и загрузка семплов в память
		OPEN_STREAMING = 2     // подготовка к декодированию
	};

	void _OpenFile(const wchar_t* pFileName, OpenMode Mode);

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

