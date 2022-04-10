#pragma once

#include "XGlobals.h"
#include "XSoundDecoder.h"
#include "XAux.h"

class XWMADecoder :	public XSoundDecoder
{
private:
	std::wstring    m_strFileName; // ��� �����
	uint32_t        m_nFileSize;   // ������ ������ (����)
	WAVEFORMATEX    m_SoundFormat; // ������ �����
	
	bool            m_bLoaded;     // TRUE, ���� �������� � ������
	bool            m_bAssigned;   // TRUE, ���� ������
	bool            m_bDecoding;   // TRUE, ���� ������������
	uint32_t        m_nRefCount;   // ������� ������

	// ������ ����� (�������� ������, �����������):
	std::unique_ptr<BYTE[]> m_upSamples;

	// ������ ��� ���������� �������������:
	std::unique_ptr<IWMSyncReader, XAux::COM_deleter_type> m_upWMReader;
	std::unique_ptr<INSSBuffer, XAux::COM_deleter_type>    m_upBuffer;
	std::unique_ptr<IStream, XAux::COM_deleter_type>       m_upStream;

	uint32_t m_nBytesRemain;
	uint32_t m_nRemainOffset;

	// ������� ��� ������������������:
	mutable std::shared_mutex m_mtxMain;

	// ����� ������ (��������������� � AssignFile():
	XSoundDecoder::AssignHint m_hintMode;

public:

	XWMADecoder();
	virtual ~XWMADecoder();

	static XSoundDecoder* CreateStatic();

private:
	
	// ������ �������� ����� ��� ������ _OpenFile()
	enum class OpenMode : uint8_t
	{
		OPEN_GENERIC      = 0, // ������� � ���������� ��������� �����
		OPEN_READ_FORMAT  = 1, // + ��������� ������ � m_SoundFormat
		OPEN_GET_SIZE     = 2, // + ���������� ������ � ���������� m_nFileSize
		OPEN_LOAD_SAMPLES = 4, // + ��������� ������ � m_upSamples
		OPEN_STREAM       = 8  // + ����������� � ���������� �������������
	};

	void _OpenFile(const wchar_t* pFileName, uint8_t Mode);

	void _ResetInternalData();

public:

	// ���������� ���������� XSoundDecoder.

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

