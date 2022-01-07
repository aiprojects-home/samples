#pragma once

#include <wmsdkidl.h>
#include <wmsdk.h>
#include <fstream>
#include <filesystem>
#include <memory>
#include <vector>
#include <list>
#include <string>
#include <shared_mutex>

enum  class XSoundFileType : uint8_t
{
	XSoundFile_WMA = 0x1,
	XSoundFile_WAV = 0x2
};

class XSoundFile
{
private:

	std::wstring    m_strFileName; // file properties
	XSoundFileType  m_FileType;
	uint32_t        m_nFileSize;
	uint16_t        m_nSoundId;
	WAVEFORMATEX    m_SoundFormat;

	bool            m_bLoaded;     // file states
	bool            m_bAssigned;
	uint32_t        m_nRefCount;  
	
	// File's data (samples).
	std::unique_ptr<BYTE[]> m_spSamples;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

public:
	
	XSoundFile();

	// Assigns sound file and gets it's properties.
	void AssignFile(const wchar_t* pFileName, uint16_t id);

	// Loads file into memory.
	bool Load();

	// Returns TRUE if file is loaded.
	bool IsLoaded() const;

	// Unloads file and frees memory if reference counter is zero.
	bool Unload();

	// Returns file id.
	uint16_t GetId() const;

	// Returns file size.
	uint32_t GetSize() const;

	// Returns file format.
	bool GetFormat(WAVEFORMATEX& wfex) const;

	// Returns copy of file data if file is loaded.
	bool GetData(std::unique_ptr<BYTE[]>& spdata) const;

	// Returns pointer to internal data if file is loaded. Also increases reference counter.
	bool GetDataDirect(BYTE*& refBuffer);

	// Dereases internal counter.
	void FreeData();

private:

	void OpenWMFile(const wchar_t* pFileNameSource, bool bLoad = false);
	void OpenWAVFile(const wchar_t* pFileNameSource, bool bLoad = false);
};

