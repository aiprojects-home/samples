#include "pch.h"
#include "XSoundBank.h"
#include "XAux.h"
#include "XEFMReader.h"
#include "XDecoderManager.h"

XSoundBank::XSoundBank(const uint32_t maxsize) : m_MaxBankSize{ maxsize }
{
	m_bAssigned = false;
	m_CurrentSize = 0;
}

void XSoundBank::AssignFile(const wchar_t* pFileName)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// ������ ��������� ��������.
		throw XException(L"XSoundBank()::AssignFile(): file is already assigned");
	}

	m_listEntries.clear();
	m_umapEntries.clear();

	try
	{
		XSoundBankParser().Parse(pFileName, &m_listEntries);
	}
	catch (const XException& e)
	{
		// ������ �� ����� ���������.
		throw XException(e, L"XSoundBank::AssignFile(): invalid data in file '%s'", pFileName);
	}

	// ���� ������������ ���������.

	for (auto it = m_listEntries.begin(); it != m_listEntries.end(); ++it)
	{
		auto &entry = *it;
		XSoundDecoder* pSoundDecoder;

		// �������� ��������� �������.
		try
		{
			pSoundDecoder = XDecoderManager::Current().OpenFile(entry.strFileName.c_str());

			entry.spDecoder.reset(pSoundDecoder);
			
			m_umapEntries[entry.nId] = it;
		}
		catch (const XException& e)
		{
			// ������ - ��� ����������.
			std::wstring s = entry.strFileName;

			m_listEntries.clear();
			m_umapEntries.clear();

			throw XException(e, L"XSoundBank::AssignFile(): can't assign sound file '%s'", s.c_str());
		}

	}

	// ��� OK.

	m_bAssigned = true;
	m_strFileName = pFileName;
}

bool XSoundBank::CanFetch(const uint16_t id) const
{
	std::shared_lock lock(m_Lock);

	auto pEntry = FindEntry(id);

	if (!pEntry)
	{
		return false;
	};

	return pEntry->bFetch;
}

bool XSoundBank::CanStream(const uint16_t id) const
{
	std::shared_lock lock(m_Lock);

	auto pEntry = FindEntry(id);

	if ((!pEntry) || (!pEntry->bStream))
	{
		return false;
	}

	return true;
}

bool XSoundBank::ReserveMemory(uint16_t nId)
{
	std::unique_lock lock(m_Lock);

	std::list<XSoundBankEntry>::iterator it;

	try
	{
		it = m_umapEntries.at(nId);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// ��� ����� ������.
		return false;
	}

	// ���������� ������ � ������ ������ (����� ������� ����� ������������ ������ ����������� � �����).
	XSoundBankEntry Entry = std::move(*it);

	m_listEntries.erase(it);
	m_listEntries.push_front(std::move(Entry));
	m_umapEntries[nId] = m_listEntries.begin();

	// ������� ���� ����� ����������, ����� �������� ����� ��������.
	uint32_t nBytesToFree = m_listEntries.front().spDecoder->GetSize();

	// ��������� �������� �������. �������� � �����.
	for (auto rev_it = m_listEntries.rbegin(); rev_it != m_listEntries.rend(); ++rev_it)
	{
		if ((nBytesToFree + m_CurrentSize) > m_MaxBankSize)
		{
			// ������ ��� ��� �� ������� -- ��������� ������.
			if (!rev_it->spDecoder->IsLoaded())
			{
				// ��� ������ ��� ���������.
				continue;
			};

			// �������� ���������.
			if (!rev_it->spDecoder->Unload())
			{
				// �� ����� �� �����-�� �������� (������� �� ����?). ���� ������.
				continue;
			};

			// ������� ����������, ������������ ������� ������.
			m_CurrentSize -= rev_it->spDecoder->GetSize();
		}
		else
		{
			// ���, ������ ������ �������.
			break;
		}
	}

	// ������ �� ���� �������. ��������: ������ ���������� ������?
	if ((nBytesToFree + m_CurrentSize) > m_MaxBankSize)
	{
		// ���, ��� ����� �� �������. �������.
		return false;
	}

	// ����������� ������.
	m_CurrentSize += nBytesToFree;

	return true;
}

bool XSoundBank::FreeMemory(uint16_t nId)
{
	std::unique_lock lock(m_Lock);

	std::list<XSoundBankEntry>::iterator it;

	try
	{
		it = m_umapEntries.at(nId);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// ��� ����� ������.
		return false;
	}

	// ������������ �������.
	m_CurrentSize -= it->spDecoder->GetSize();

	return true;
};

XSoundDecoder* XSoundBank::GetDecoder(uint16_t nId) const
{
	std::shared_lock lock(m_Lock);

	std::list<XSoundBankEntry>::iterator it;

	try
	{
		it = m_umapEntries.at(nId);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// ��� ����� ������.
		return nullptr;
	}

	return it->spDecoder.get();

}

XSoundBankEntry* XSoundBank::FindEntry(uint16_t id) const
{
	try
	{
		auto &e = *(m_umapEntries.at(id));

		return &e;

	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		return nullptr;
	}
}
