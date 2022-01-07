#include "XEFM.h"
#include "framework.h"
#include "XException.h"

#include <filesystem>
#include <fstream>

std::unique_ptr<XEFM> XEFM::m_spCurrent{ nullptr };

// Storage header:
using XEFMHeaderType = struct XEFM_HEADER
{
	char     Label[5];   // 'EFILE'
	uint8_t  VerHi;      // 2
	uint8_t  VerLo;      // 2
	uint32_t NumEntries; // entries count
	uint32_t DataSize;   // sum of all files

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
	if (m_spCurrent == nullptr)
	{
		m_spCurrent = std::unique_ptr<XEFM>(new XEFM());
	}

	return *m_spCurrent.get();
}

void XEFM::CreateStorage(const wchar_t* pDir, const wchar_t* pStorage)
{
	// File entry: real file name, map name, offset, size:
	using FileEntry = std::tuple<std::wstring, std::wstring, uint32_t, uint32_t>;

	if (!std::filesystem::path(pDir).is_absolute())
	{
		// Path must be absolute.
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

		// Iterating through all directories:
		for (const auto& e : dir_it)
		{
			if (e.is_regular_file())
			{
				// New file found. Try to add it.
				strFilePath = e.path().c_str();

				if (strFilePath.find(strRootPath) != 0)
				{
					throw XException(L"XEFM::CreateStorage(): unable to process file '%s'", strFilePath.c_str());
				}

				// Replacing root path with slash to get map file name:
				strFileMapName = strFilePath;
				strFileMapName.replace(strFileMapName.begin(), strFileMapName.begin() + strRootPath.length(), L"");
				
				// Getting file size:
				nFileSize = (uint32_t)std::filesystem::file_size(e.path());

				// Adding entry:
				listFiles.emplace_back(strFilePath, strFileMapName, nFileOffset, nFileSize);
				
				// Adjusting offset:
				nFileOffset += nFileSize;
				nTotalSize += nFileSize;
			}
		}

		// Now try to create storage file.
		std::ofstream stgFile{ pStorage, std::ios::out | std::ios::binary |std::ios::trunc };

		if (!stgFile.is_open())
		{
			throw XException(L"XEFM::CreateStorage(): can't create storage file '%s'", pStorage);
		}

		stgFile.unsetf(std::ios::skipws);

		// Writing header.
		XEFMHeaderType header{(uint32_t)listFiles.size(), nTotalSize };

		auto x = sizeof(XEFMHeaderType);

		stgFile.write(reinterpret_cast<char*>(&header), sizeof(XEFMHeaderType));

		// Copying contents.
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

		// Writing data: (map name, offset, size)
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
					// Write each char:
					auto c = s[i];

					stgFile.write(reinterpret_cast<char*>(&c), sizeof(wchar_t));
				}

				// NULL in the end:
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

	// All is done.
}

void XEFM::AssignFile(const wchar_t* pStorageFile)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// Can't assign twice.
		throw XException(L"XEFM::AssignFile(): Storage file is already assigned");
	}

	std::ifstream stgFile{ pStorageFile, std::ios::in | std::ios::binary };

	if (!stgFile.is_open())
	{
		// Can't open file.
		throw XException(L"XEFM::AssignFile(): can't open file '%s'", pStorageFile);
	}

	// Reading header:
	XEFMHeaderType header{ 0, 0 };

	stgFile.read(reinterpret_cast<char*>(&header), sizeof(XEFMHeaderType));

	if (!header.IsValid())
	{
		// Header is broken.
		throw XException(L"XEFM::AssignFile(): unsupported file type, header is not valid");
	}

	// Seeking to map:
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
				// Counting chars until '\0'.
				stgFile.read(reinterpret_cast<char*>(&c), sizeof(wchar_t));
				l++;
			} while (c != '\0');

			// Restoring position & loading string.
			stgFile.seekg(old_pos);

			std::unique_ptr<wchar_t[]> spBuffer{ new wchar_t[l] };
			stgFile.read(reinterpret_cast<char*>(spBuffer.get()), l * sizeof(wchar_t));

			s.assign(spBuffer.get(), spBuffer.get() + l - 1);
		};

		std::wstring strFileMapName;
		uint32_t     nFileOffset, nFileSize;

		// Reading entry:

		load_string(strFileMapName);
		stgFile.read(reinterpret_cast<char*>(&nFileOffset), sizeof(uint32_t));
		stgFile.read(reinterpret_cast<char*>(&nFileSize), sizeof(uint32_t));

		// Storing in the map:
		
		m_FileMap[strFileMapName] = std::pair(nFileOffset, nFileSize);
	}

	// Everything is OK.

	m_bAssigned = true;
	m_StorageFileName = pStorageFile;
	m_StoragePath = m_StorageFileName.substr(0, m_StorageFileName.find_last_of('\\') + 1);

	// Changing current path to storage location and getting path without filename.
	std::filesystem::current_path(m_StoragePath);
	m_StoragePath = std::filesystem::current_path();
}

void XEFM::SetExtendedMode(bool mode)
{
	std::unique_lock lock{ m_Lock };

	m_bExtendedMode = mode;
}


void XEFM::GetReaderInfo(const wchar_t* pFileName, std::wstring& strContainer, uint32_t& nStartPos, uint32_t& nEndPos) const
{
	std::shared_lock lock{ m_Lock };

	if (!m_bAssigned)
	{
		// Not assigned.
		throw XException(L"XEFM::GetReaderInfo(): Storage file is not assigned");
	}

	auto pos_it = m_FileMap.find(std::wstring(pFileName));

	if ( (pos_it == m_FileMap.end()) && (!m_bExtendedMode))
	{
		// No such filename. Extended mode is off.
		throw XException(L"XEFM::GetReaderInfo(): File '%s' not found", pFileName);
	}
	
	// First, try to open REAL file:
	std::wstring strRealName = m_StoragePath + std::wstring(pFileName);
	bool bRealExists{ false };
	
	{
		std::ifstream file{ strRealName, std::ios::in };

		bRealExists = file.is_open();
	}

	if (bRealExists)
	{
		// Real file exists. Return it.
		strContainer = strRealName;
		nStartPos = 0;
		nEndPos = ((uint32_t)std::filesystem::file_size(strRealName)) - 1;
	}
	else
	{
		if ((pos_it == m_FileMap.end()))
		{
			// Extended mode is on, but there is no real file and there is no file in the storage.
			throw XException(L"XEFM::GetReaderInfo(): File '%s' not found", pFileName);
		};

		// Getting file info from internal map.
		const auto[nFileOffset, nFileSize] = m_FileMap.at(pFileName);

		// Return storage info.
		strContainer = m_StorageFileName;
		nStartPos = nFileOffset;
		nEndPos = nFileOffset + nFileSize - 1;
	}
}
