#pragma once

#include "XSoundDecoder.h"
#include <shared_mutex>
#include "XAux.h"

class XPCMDecoder :	public XSoundDecoder
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
	uint32_t m_nBytesRemain;
	uint32_t m_nCurrentOffset;
	uint32_t m_nDataOffset;

	// ������� ��� ������������������:
	mutable std::shared_mutex m_mtxMain;

public:
	XPCMDecoder();
	virtual ~XPCMDecoder();

	static XSoundDecoder* CreateStatic();

private:

	void _OpenFile(const wchar_t* pFileName, bool bLoad = false);

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

