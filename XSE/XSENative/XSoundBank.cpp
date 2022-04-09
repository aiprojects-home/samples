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
		// Нельзя назначить повторно.
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
		// Ошибка во время обработки.
		throw XException(e, L"XSoundBank::AssignFile(): invalid data in file '%s'", pFileName);
	}

	// Файл благополучно обработан.

	for (auto it = m_listEntries.begin(); it != m_listEntries.end(); ++it)
	{
		auto &entry = *it;
		XSoundDecoder* pSoundDecoder;

		// Пытаемся подобрать декодер.
		try
		{
			pSoundDecoder = XDecoderManager::Current().OpenFile(entry.strFileName.c_str());

			entry.spDecoder.reset(pSoundDecoder);
			
			m_umapEntries[entry.nId] = it;
		}
		catch (const XException& e)
		{
			// Ошибка - все сбрасываем.
			std::wstring s = entry.strFileName;

			m_listEntries.clear();
			m_umapEntries.clear();

			throw XException(e, L"XSoundBank::AssignFile(): can't assign sound file '%s'", s.c_str());
		}

	}

	// Все OK.

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

		// Нет такой записи.
		return false;
	}

	// Перемещаем запись в начало списка (таким образом редко используемые записи оказываются в конце).
	XSoundBankEntry Entry = std::move(*it);

	m_listEntries.erase(it);
	m_listEntries.push_front(std::move(Entry));
	m_umapEntries[nId] = m_listEntries.begin();

	// Сколько байт нужно освободить, чтобы загрузка стала возможна.
	uint32_t nBytesToFree = m_listEntries.front().spDecoder->GetSize();

	// Процедура выгрузки семплов. Начинаем с конца.
	for (auto rev_it = m_listEntries.rbegin(); rev_it != m_listEntries.rend(); ++rev_it)
	{
		if ((nBytesToFree + m_CurrentSize) > m_MaxBankSize)
		{
			// Памяти все еще не хватает -- выгружаем дальше.
			if (!rev_it->spDecoder->IsLoaded())
			{
				// Эта запись уже выгружена.
				continue;
			};

			// Пытаемся выгрузить.
			if (!rev_it->spDecoder->Unload())
			{
				// Не вышло по каким-то причинам (счетчик не ноль?). Идем дальше.
				continue;
			};

			// Немного освободили, корректируем счетчик памяти.
			m_CurrentSize -= rev_it->spDecoder->GetSize();
		}
		else
		{
			// Все, теперь памяти хватает.
			break;
		}
	}

	// Прошли по всем записям. Проверка: теперь достаточно памяти?
	if ((nBytesToFree + m_CurrentSize) > m_MaxBankSize)
	{
		// Нет, все равно не хватает. Выходим.
		return false;
	}

	// Резервируем память.
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

		// Нет такой записи.
		return false;
	}

	// Корректируем счетчик.
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

		// Нет такой записи.
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
