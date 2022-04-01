#pragma once

#include "XSoundDecoder.h"
#include <wmsdk.h>
#include <shared_mutex>
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

public:
	XWMADecoder();
	virtual ~XWMADecoder();

	static XSoundDecoder* CreateStatic();

private:
	
	// ������ �������� ����� ��� ������ _OpenFile()
	enum class OpenMode : uint8_t
	{
		OPEN_ANALYSE = 0,      // ����������� �������
		OPEN_LOAD_SAMPLES = 1, // ����������� ������� � �������� ������� � ������
		OPEN_STREAMING = 2     // ���������� � �������������
	};

	void _OpenFile(const wchar_t* pFileName, OpenMode Mode);

	void _ResetInternalData();

public:

	// ���������� ���������� XSoundDecoder.

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

