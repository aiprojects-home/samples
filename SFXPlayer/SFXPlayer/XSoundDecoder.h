#pragma once

#include "framework.h"
#include <memory>

class XSoundDecoder
{
public:
	virtual ~XSoundDecoder();

	// ����� ��������� ��������� ����.
	virtual void AssignFile(const wchar_t* pFileName) = 0;
    
	// ����� ���������� ������� ����������� ���� � ����������� �������.
	virtual bool ReleaseFile() = 0;

	// ����� ���������� TRUE, ���� ���� ������.
	virtual bool IsAssigned() = 0;

	// ����� ��������� ������ �� ����� ������� � ������. ���� ����������, ���������� ����������.
	virtual bool Load() = 0;
	
	// ����� ��������� ������ �� ������, ���������� ��, ���� ������� ��������� � ������ ����� 0.
	virtual bool Unload() = 0;

	// ����� ���������� TRUE ���� ���� ��������.
	virtual bool IsLoaded() = 0;

	// ����� ���������� ������ ����������� ������.
	virtual uint32_t GetSize() = 0;

	// ����� ���������� ������ ��������� �����.
	virtual bool GetFormat(WAVEFORMATEX & refFormat) = 0;

	// ����� ���������� ����� ������.
	virtual bool GetData(std::unique_ptr<BYTE[]> &refData) = 0;
	
	// ����� ���������� ������ ��������� �� ����������� ������ � ����������� ������� ������.
	virtual bool GetDataDirect(BYTE*& refData) = 0;
	
	// ����� ���������� ���������� ������� ��������� � ������.
	virtual void FreeData() = 0;

	// ����� �������������� �������� ���� � �������������.
	virtual void DecodeStart(WAVEFORMATEX &wfex) = 0;
	
	// ����� ������������� ��������� ������ ������.
	virtual uint32_t DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount) = 0;

	// ����� ������������� ������������� � ����������� �������.
	virtual bool DecodeStop() = 0;

	// ����� ���������� TRUE, ���� ����� ������� �������������.
	virtual bool IsDecoding() = 0;

};

typedef XSoundDecoder* CreateDecoderFunc();

