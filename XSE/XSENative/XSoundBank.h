#pragma once

#include "XGlobals.h"
#include "XSoundDecoder.h"
#include "XSoundBankParser.h"

class XSoundBank
{
private:

	std::wstring    m_strFileName; // �������� �����
	bool            m_bAssigned;
	const uint32_t  m_MaxBankSize;
	uint32_t        m_CurrentSize;

	// ��������� ��������� ��� �������� ������. �������� �������� � ������, � ��� �������� �������
	// � ��� ������������ ���-������� � ��������������� �������, ������� ������ �������� �� ������
	// ������� � ������. Map Id -> iterator -> list entry.
	// ����� ������ ������������ ��� ����, ����� ������� ����������� ����������� ������������
	// std::list - ��� �������� ����� ������ (+ ����������� ����������� ������ ������ � ������)
	// std::umap - ��� �������� ������� � �������� ������ �� ��������� (ID -> it)
	std::list<XSoundBankEntry> m_listEntries;
	std::unordered_map<uint16_t, std::list<XSoundBankEntry>::iterator> m_umapEntries;

	// ������� ��� ������������������.
	mutable std::shared_mutex m_Lock;

public:
	XSoundBank(const uint32_t maxsize = 0x100000 /*1MB ��-���������*/);

	// ������� ���� �� ����� ��������.
	void AssignFile(const wchar_t* pFileName);

	// ���������� TRUE ���� ���� ����� ��������� � ������ (����������� ��������������� ��� ��������).
	bool CanFetch(const uint16_t id) const;

	// ���������� TRUE ���� ���� ����� �������������� � ������ (��� ������).
	bool CanStream(const uint16_t id) const;

	// �������� ��������������� ������ ��� �������� ����� �� ��� ID. ��� ���� ����������� ������ � ������.
	bool ReserveMemory(uint16_t nId);

	// ������������� �������� ������: �� ����������� �� ������ ����� � �������� ID.
	bool FreeMemory(uint16_t nId);

	// ��������� �������� ��� ����� �� ID.
	XSoundDecoder* GetDecoder(uint16_t nId) const;

private:

	XSoundBankEntry* FindEntry(uint16_t id) const;
};

