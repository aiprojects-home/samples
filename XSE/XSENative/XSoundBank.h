#pragma once

#include "XGlobals.h"
#include "XSoundDecoder.h"
#include "XSoundBankParser.h"

class XSoundBank
{
private:

	std::wstring    m_strFileName; // свойства банка
	bool            m_bAssigned;
	const uint32_t  m_MaxBankSize;
	uint32_t        m_CurrentSize;

	// Коллекция декодеров для звуковых файлов. Декодеры хранятся в списке, а для быстрого доступа
	// к ним используется хеш-таблица с идентификатором эффекта, которая хранит итератор на нужный
	// элемент в списке. Map Id -> iterator -> list entry.
	// Такой способ используется для того, чтобы создать возможность выталкивать используемые
	// std::list - для хранения самой записи (+ возможность выталкивать нужную запись в начало)
	// std::umap - для быстрого доступа к элементу списка по итератору (ID -> it)
	std::list<XSoundBankEntry> m_listEntries;
	std::unordered_map<uint16_t, std::list<XSoundBankEntry>::iterator> m_umapEntries;

	// Мьютекс для потокобезопасности.
	mutable std::shared_mutex m_Lock;

public:
	XSoundBank(const uint32_t maxsize = 0x100000 /*1MB по-умолчанию*/);

	// Создать банк из файла настроек.
	void AssignFile(const wchar_t* pFileName);

	// Возвращает TRUE если файл можно загрузить в память (стандартное воспроизведение для эффектов).
	bool CanFetch(const uint16_t id) const;

	// Возвращает TRUE если файл можно воспроизводить в потоке (для музыки).
	bool CanStream(const uint16_t id) const;

	// Пытается зарезервировать память для загрузки файла по его ID. При этом выталкивает запись в начало.
	bool ReserveMemory(uint16_t nId);

	// Корректировка счетчика памяти: он уменьшается на размер файла с заданным ID.
	bool FreeMemory(uint16_t nId);

	// Получение декодера для файла по ID.
	XSoundDecoder* GetDecoder(uint16_t nId) const;

private:

	XSoundBankEntry* FindEntry(uint16_t id) const;
};

