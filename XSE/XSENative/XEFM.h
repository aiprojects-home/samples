#pragma once

#include "XGlobals.h"

class XEFM
{
	friend class XEFMReader;
private:
	bool         m_bAssigned;        // �������� ���������
	std::wstring m_StorageFileName;
	std::wstring m_StoragePath;
	bool         m_bExtendedMode;

	// ���������� ������: ��� ����� -> �������� ����� & �����.
	std::unordered_map<std::wstring, std::pair<uint32_t, uint32_t>> m_FileMap;

	// ������ � ������������ ����������.
	static std::unique_ptr<XEFM> m_upCurrent;

	// �������� ��� ������������������.
	mutable std::shared_mutex m_Lock;
	static  std::mutex        m_StaticLock;

	XEFM();

public:
	static XEFM& Current();

	// ������� ��������� �� ���� ������ �� ��������� ����.
	static void CreateStorage(const wchar_t* pDir, const wchar_t* pStorage);

	// ���������� ����� - ����������.
	void AssignFile(const wchar_t* pStorageFile);

	// ��������� ������������ ������ (��������� ������ ������ �� FAT �������� �����, ������� ����������� � ���������.
	void SetExtendedMode(bool mode);

	// ����� ������� �������� � ������� ������.
	void Reset();

private:
	// ������������� �������� ������ ��� ���������� ����� � ���������.
	void GetReaderInfo(const wchar_t* pFileName, std::wstring& strContainer, uint32_t& nStartPos, uint32_t& nEndPos) const;
};

