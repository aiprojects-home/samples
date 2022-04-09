#pragma once

#include "XGlobals.h"

class XEFM
{
	friend class XEFMReader;
private:
	bool         m_bAssigned;        // свойства хранилища
	std::wstring m_StorageFileName;
	std::wstring m_StoragePath;
	bool         m_bExtendedMode;

	// Внутренние данные: имя файла -> смещение файла & длина.
	std::unordered_map<std::wstring, std::pair<uint32_t, uint32_t>> m_FileMap;

	// Объект в единственном экземпляре.
	static std::unique_ptr<XEFM> m_upCurrent;

	// Мьютексы для потокобезопасности.
	mutable std::shared_mutex m_Lock;
	static  std::mutex        m_StaticLock;

	XEFM();

public:
	static XEFM& Current();

	// Создает контейнер из всех файлов по заданному пути.
	static void CreateStorage(const wchar_t* pDir, const wchar_t* pStorage);

	// Назначение файла - контейнера.
	void AssignFile(const wchar_t* pStorageFile);

	// Активация расширенного режима (позволяет ридеру читать из FAT реальные файлы, которые отсутствуют в хранилище.
	void SetExtendedMode(bool mode);

	// Сброс текущих настроек и очистка данных.
	void Reset();

private:
	// Устанавливает свойства ридера для указанного файла в хранилище.
	void GetReaderInfo(const wchar_t* pFileName, std::wstring& strContainer, uint32_t& nStartPos, uint32_t& nEndPos) const;
};

