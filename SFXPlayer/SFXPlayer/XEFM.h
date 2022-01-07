#pragma once

#include <string>
#include <memory.h>
#include <shared_mutex>
#include <unordered_map>

class XEFM
{
	friend class XEFMReader;
private:
	bool         m_bAssigned;        // storage properties
	std::wstring m_StorageFileName;
	std::wstring m_StoragePath;
	bool         m_bExtendedMode;

	// Internal data: filename -> file offset & file length.
	std::unordered_map<std::wstring, std::pair<uint32_t, uint32_t>> m_FileMap;

	// The only copy of the object.
	static std::unique_ptr<XEFM> m_spCurrent;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

	XEFM();

public:
	static XEFM& Current();

	// Creates storage file from contents of given folder.
	static void CreateStorage(const wchar_t* pDir, const wchar_t* pStorage);

	// Assigns storage file.
	void AssignFile(const wchar_t* pStorageFile);

	// Sets or removes extended mode flag (enables XEFMReader to open files that don't exist in the storage)
	void SetExtendedMode(bool mode);

private:
	// Sets reader properties for given file name.
	void GetReaderInfo(const wchar_t* pFileName, std::wstring& strContainer, uint32_t& nStartPos, uint32_t& nEndPos) const;
};

