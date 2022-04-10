#pragma once

#include "XGlobals.h"
#include "XSoundDecoder.h"
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

	// Режим работы (устанавливается в AssignFile():
	XSoundDecoder::AssignHint m_hintMode;

public:

	XWMADecoder();
	virtual ~XWMADecoder();

	static XSoundDecoder* CreateStatic();

private:
	
	// Режимы загрузки файла для метода _OpenFile()
	enum class OpenMode : uint8_t
	{
		OPEN_GENERIC      = 0, // открыть и попытаться подобрать кодек
		OPEN_READ_FORMAT  = 1, // + прочитать формат в m_SoundFormat
		OPEN_GET_SIZE     = 2, // + определить размер и установить m_nFileSize
		OPEN_LOAD_SAMPLES = 4, // + загрузить семплы в m_upSamples
		OPEN_STREAM       = 8  // + подготовить к потоковому декодированию
	};

	void _OpenFile(const wchar_t* pFileName, uint8_t Mode);

	void _ResetInternalData();

public:

	// Реализация интерфейса XSoundDecoder.

	virtual void AssignFile(const wchar_t* pFileName, XSoundDecoder::AssignHint Hint);
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
	virtual void DecodeStart();
	virtual uint32_t DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount);
	virtual bool DecodeStop();
	virtual bool IsDecoding();
};

