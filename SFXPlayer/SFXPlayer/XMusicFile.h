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

#include "XAux.h"

class XMusicFile
{
private:

	std::wstring    m_strFileName; // file properties
	uint32_t        m_nFileSize;
	uint16_t        m_nMusicId;
	WAVEFORMATEX    m_MusicFormat;

	bool            m_bAssigned;   // file states
	bool            m_bDecoding;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

	// Decoding data.
	std::unique_ptr<IWMSyncReader, XAux::COM_deleter_type> m_spWMReader;
	std::unique_ptr<INSSBuffer, XAux::COM_deleter_type>    m_spBuffer;
	std::unique_ptr<IStream, XAux::COM_deleter_type>       m_spStream;

	uint32_t m_nBytesRemain;
	uint32_t m_nRemainOffset;

public:

	XMusicFile();
	~XMusicFile();

	// Assigns music file.
	void AssignFile(const wchar_t* pFileName, const uint16_t id);

	// Prepares file for decoding.
	bool DecodeStart(WAVEFORMATEX &wfex);

	// Decodes 'count' bytes into given buffer. Returns count of actually decoded bytes.
	uint32_t DecodeBytes(std::unique_ptr<uint8_t[]>& spDestBuffer, const uint32_t count);

	// Finishes decoding. Frees memory.
	bool DecodeStop();

	// Returns decoding status.
	bool IsDecoding() const;

private:

};


