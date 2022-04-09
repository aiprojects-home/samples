#include "pch.h"
#include "XEFM.h"
#include "XException.h"

std::unique_ptr<XEFM> XEFM::m_upCurrent{ nullptr };
std::mutex            XEFM::m_StaticLock;

// Заголовок контейнера:
using XEFMHeaderType = struct XEFM_HEADER
{
	char     Label[5];   // 'EFILE'
	uint8_t  VerHi;      // 2
	uint8_t  VerLo;      // 2
	uint32_t NumEntries; // количество файлов внутри
	uint32_t DataSize;   // общая длинна всех файлов

	XEFM_HEADER(uint32_t entries, uint32_t size)
	{
		char s[6] = "EFILE";

		std::copy(s, s + 5, Label);
		VerHi = 2;
		VerLo = 2;
		NumEntries = entries;
		DataSize = size;
	}

	bool IsValid()
	{
		// Проверка на корректность заголовка.
		if ( (strncmp(Label, "EFILE", 5)) || (VerHi != 2) || (VerLo != 2))
		{
			return false;
		}

		return true;
	}
};

XEFM::XEFM()
{
	m_bAssigned = false;
	m_bExtendedMode = false;
}

XEFM& XEFM::Current()
{
	std::unique_lock lock{ m_StaticLock };

	if (m_upCurrent == nullptr)
	{
		m_upCurrent = std::unique_ptr<XEFM>(new XEFM());
	}

	return *m_upCurrent.get();
}

void XEFM::CreateStorage(const wchar_t* pDir, const wchar_t* pStorage)
{
	std::unique_lock lock{ m_StaticLock };

	// Запись: имя файла в FAT, имя файла в хеш-таблице, размер:
	using FileEntry = std::tuple<std::wstring, std::wstring, uint32_t, uint32_t>;

	if (!std::filesystem::path(pDir).is_absolute())
	{
		// Путь должен быть абсолютным.
		throw XException(L"XEFM::CreateStorage(): invalid path '%s' (must be absolute)", pDir);
	}

	std::filesystem::current_path(pDir);

	const std::wstring strRootPath = std::filesystem::current_path();

	try
	{
		std::filesystem::recursive_directory_iterator dir_it{ std::filesystem::path{strRootPath} };
   		std::list<FileEntry>                          listFiles;
		
		std::wstring strFilePath;
		std::wstring strFileMapName;
		uint32_t     nFileSize{ 0 };
		uint32_t     nFileOffset{ sizeof(XEFMHeaderType) };
		uint32_t     nTotalSize{ 0 };

		// Цикл по всем вложенным папкам. Ищем файлы и формируем записи.
		for (const auto& e : dir_it)
		{
			if (e.is_regular_file())
			{
				// Нашли новый файл, пытаемся добавить.
				strFilePath = e.path().c_str();

				if (strFilePath.find(strRootPath) != 0)
				{
					throw XException(L"XEFM::CreateStorage(): unable to process file '%s'", strFilePath.c_str());
				}

				// Затираем часть пути чтобы получить имя файла:
				strFileMapName = strFilePath;
				strFileMapName.replace(strFileMapName.begin(), strFileMapName.begin() + strRootPath.length(), L"");
				
				// Получаем размер файла:
				nFileSize = (uint32_t)std::filesystem::file_size(e.path());

				// Добавляем запись:
				listFiles.emplace_back(strFilePath, strFileMapName, nFileOffset, nFileSize);
				
				// Корректируем смещение:
				nFileOffset += nFileSize;
				nTotalSize += nFileSize;
			}
		}

		// Теперь пробуем создать новое хранилище.
		std::ofstream stgFile{ pStorage, std::ios::out | std::ios::binary |std::ios::trunc };

		if (!stgFile.is_open())
		{
			throw XException(L"XEFM::CreateStorage(): can't create storage file '%s'", pStorage);
		}

		stgFile.unsetf(std::ios::skipws);

		// Пишем заголовок.
		XEFMHeaderType header{(uint32_t)listFiles.size(), nTotalSize };

		auto x = sizeof(XEFMHeaderType);

		stgFile.write(reinterpret_cast<char*>(&header), sizeof(XEFMHeaderType));

		// Копируем данные (содержимое файлов).
		for (const auto& f : listFiles)
		{
			std::ifstream srcFile{ std::get<0>(f), std::ios::binary |std::ios::in};

			if (!srcFile.is_open())
			{
				throw XException(L"XEFM::CreateStorage(): can't open source file 's'", std::get<0>(f));
			}

			srcFile.unsetf(std::ios::skipws);

			for(char d;;)
			{
				srcFile >> d;

				if (srcFile.eof())
					break;
				
				stgFile << d;
			}
    	}

		// Пишем данные о структуре хранилища в конец файла - контейнера (имя, смещение и размер).
		for (const auto& f : listFiles)
		{
			strFileMapName = std::get<1>(f);
			nFileOffset = std::get<2>(f);
			nFileSize = std::get<3>(f);

			auto store_string = [&stgFile](const std::wstring& s)
			{
				wchar_t send = '\0';

				for (size_t i = 0; i < s.size(); ++i)
				{
					// Пишем каждый символ:
					auto c = s[i];

					stgFile.write(reinterpret_cast<char*>(&c), sizeof(wchar_t));
				}

				// NULL в конце:
				stgFile.write(reinterpret_cast<char*>(&send), sizeof(wchar_t));
			};

			store_string(strFileMapName);
			stgFile.write(reinterpret_cast<char*>(&nFileOffset), sizeof(uint32_t));
			stgFile.write(reinterpret_cast<char*>(&nFileSize), sizeof(uint32_t));
		};

	}
	catch (const std::exception& e)
	{
		UNREFERENCED_PARAMETER(e);

		throw XException(L"XEFM::CreateStorage(): internal error");
	}

	// Закончили.
}

void XEFM::AssignFile(const wchar_t* pStorageFile)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// Назначать можно только один раз.
		throw XException(L"XEFM::AssignFile(): Storage file is already assigned");
	}

	std::ifstream stgFile{ pStorageFile, std::ios::in | std::ios::binary };

	if (!stgFile.is_open())
	{
		// Контейнер не открывается.
		throw XException(L"XEFM::AssignFile(): can't open file '%s'", pStorageFile);
	}

	// Читаем заголовок:
	XEFMHeaderType header{ 0, 0 };

	stgFile.read(reinterpret_cast<char*>(&header), sizeof(XEFMHeaderType));

	if (!header.IsValid())
	{
		// Заголовок некорректный.
		throw XException(L"XEFM::AssignFile(): unsupported file type, header is not valid");
	}

	// Смещаемся в конец, чтобы прочитать внутреннюю структуру:
	stgFile.seekg(header.DataSize + sizeof(XEFMHeaderType));

	for (uint32_t i = 0; i < header.NumEntries; ++i)
	{
		auto load_string = [&stgFile](std::wstring& s)
		{
			auto               old_pos = stgFile.tellg();
			wchar_t            c;
			uint32_t           l{ 0 };

			do
			{
				// Считаем сколько символов до '\0'.
				stgFile.read(reinterpret_cast<char*>(&c), sizeof(wchar_t));
				l++;
			} while (c != '\0');

			// Возвращаемся назад и читаем всю строку.
			stgFile.seekg(old_pos);

			std::unique_ptr<wchar_t[]> spBuffer{ new wchar_t[l] };
			stgFile.read(reinterpret_cast<char*>(spBuffer.get()), l * sizeof(wchar_t));

			s.assign(spBuffer.get(), spBuffer.get() + l - 1);
		};

		std::wstring strFileMapName;
		uint32_t     nFileOffset, nFileSize;

		// Читаем запись:

		load_string(strFileMapName);
		stgFile.read(reinterpret_cast<char*>(&nFileOffset), sizeof(uint32_t));
		stgFile.read(reinterpret_cast<char*>(&nFileSize), sizeof(uint32_t));

		// Сохраняем в хеш-таблице:
		
		m_FileMap[strFileMapName] = std::pair(nFileOffset, nFileSize);
	}

	//Все OK.

	m_bAssigned = true;
	m_StorageFileName = pStorageFile;
	m_StoragePath = m_StorageFileName.substr(0, m_StorageFileName.find_last_of('\\') + 1);

	// Меняем текущий путь к контейнеру и снова получаем его, но уже без имени.
	std::filesystem::current_path(m_StoragePath);
	m_StoragePath = std::filesystem::current_path();
}

void XEFM::SetExtendedMode(bool mode)
{
	std::unique_lock lock{ m_Lock };

	m_bExtendedMode = mode;
}

void XEFM::Reset()
{
	if (!m_bAssigned)
	{
		// Не назначен, нечего сбрасывать.
		return;
	}

	m_FileMap.clear();
	m_bAssigned = false;
	m_bExtendedMode = false;
}


void XEFM::GetReaderInfo(const wchar_t* pFileName, std::wstring& strContainer, uint32_t& nStartPos, uint32_t& nEndPos) const
{
	std::shared_lock lock{ m_Lock };

	if (!m_bAssigned)
	{
		// Не назначен.
		throw XException(L"XEFM::GetReaderInfo(): Storage file is not assigned");
	}

	auto pos_it = m_FileMap.find(std::wstring(pFileName));

	if ( (pos_it == m_FileMap.end()) && (!m_bExtendedMode))
	{
		// Нет такого файла, а расширенный режим выключен.
		throw XException(L"XEFM::GetReaderInfo(): File '%s' not found", pFileName);
	}
	
	// Сначала пытаемся открыть файл в FAT:
	std::wstring strRealName = m_StoragePath + std::wstring(pFileName);
	bool bRealExists{ false };
	
	{
		std::ifstream file{ strRealName, std::ios::in };

		bRealExists = file.is_open();
	}

	if (bRealExists)
	{
		// Файл есть в FAT, его и вернем.
		strContainer = strRealName;
		nStartPos = 0;
		nEndPos = ((uint32_t)std::filesystem::file_size(strRealName)) - 1;
	}
	else
	{
		if ((pos_it == m_FileMap.end()))
		{
			// Расширенный режим включен, но реального файла нет, как нет и файла в контейнере.
			throw XException(L"XEFM::GetReaderInfo(): File '%s' not found", pFileName);
		};

		// Будем возвращать файл из контейнера.
		const auto[nFileOffset, nFileSize] = m_FileMap.at(pFileName);

		// Заполняем данные.
		strContainer = m_StorageFileName;
		nStartPos = nFileOffset;
		nEndPos = nFileOffset + nFileSize - 1;
	}
}
