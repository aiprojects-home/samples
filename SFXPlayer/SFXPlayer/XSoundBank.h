#pragma once

#include <unordered_map>
#include <list>
#include <shared_mutex>

#include "XSoundDecoder.h"
#include "XSoundBankParser.h"

class XSoundBank
{
private:

	// ��������� ��������� ��� �������� ������. �������� �������� � ������, � ��� �������� �������
	// � ��� ������������ ���-������� � ��������������� �������, ������� ������ �������� �� ������
	// ������� � ������. Map Id -> iterator -> list entry.
	// ����� ������ ������������ ��� ����, ����� ������� ����������� ����������� ������������
	// ������� � ������ ������, �������� ��, ��� ��������� � �����.
	std::list<std::unique_ptr<XSoundDecoder>> m_XSoundList;
	std::unordered_map<uint16_t, std::list<std::unique_ptr<XSoundDecoder>>::iterator> m_XSoundPos;

	// ��������� ��������� ��� ���������� ���������������.
	std::unordered_map<uint16_t, std::unique_ptr<XSoundDecoder>> m_XMusicTable;


	std::wstring    m_strFileName; // bank properties
	bool            m_bAssigned;
	const uint32_t  m_MaxBankSize;
	uint32_t        m_CurrentSize;

	// ������ ��������� � �����.
	// std::list - ��� �������� ����� ������ (+ ����������� ����������� ������ ������ � ������)
	// std::umap - ��� �������� ������� � �������� ������ �� ��������� (ID -> it)
	std::list<XSoundBankEntry> m_listEntries;
	std::unordered_map<uint16_t, std::list<XSoundBankEntry>::iterator> m_umapEntries;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

public:
	XSoundBank(const uint32_t maxsize = 0x100000 /*1MB default size*/);

	// Assigns bank file to the object.
	void AssignFile(const wchar_t* pFileName);

	// Returns TRUE if sound file with given id can be fetched.
	bool CanFetch(const uint16_t id) const;

	// Returns TRUE if music file with given id can be streamed.
	bool CanStream(const uint16_t id) const;

	bool ReserveMemory(uint16_t nId);
	bool FreeMemory(uint16_t nId);

	XSoundDecoder* GetDecoder(uint16_t nId);

private:

	XSoundBankEntry* FindEntry(uint16_t id) const;
};

