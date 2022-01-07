#pragma once

#include <unordered_map>
#include <list>
#include <shared_mutex>

#include "XParserBase.h"
#include "XSoundFile.h"
#include "XMusicFile.h"

class XSoundBank
{
private:
	class XSoundBankParser : public XStringParser
	{
		friend class XSoundBank;
	private:
		const XSoundBank* m_pOwner;
		std::unordered_map<uint16_t, std::wstring> m_SFXCol;
		std::unordered_map<uint16_t, std::wstring> m_MusicCol;

	public:
		XSoundBankParser(const XSoundBank *pOwner);
		void Parse(const wchar_t* pFileName);

		static bool SFX_FILE_Handler(const XStringParser& obj, const std::wsmatch& match);
		static bool MUSIC_FILE_Handler(const XStringParser& obj, const std::wsmatch& match);

	} m_Parser;

	// Collection of XSoundFile objects & iterator's hash-table (for quick access to the list):
	// id -> iterator (hash-table) -> XSoundFile ptr (list)
	std::list<std::unique_ptr<XSoundFile>> m_XSoundFileList;
	std::unordered_map<uint16_t, std::list<std::unique_ptr<XSoundFile>>::iterator> m_XSoundFilePos;

	// Collection of XMusicFile objects:
	std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>> m_XMusicFileTable;

	std::wstring    m_strFileName; // bank properties
	bool            m_bAssigned;
	const uint32_t  m_MaxBankSize;
	uint32_t        m_CurrentSize;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

public:
	XSoundBank(const uint32_t maxsize = 0x100000 /*1MB default size*/);

	// Assigns bank file to the object.
	void AssignFile(const wchar_t* pFileName);

	// Returns TRUE if sound file with given id can be fetched.
	bool CanFetch(const uint16_t id) const;

	// Fetches file with given id.
	bool Fetch(const uint16_t id, WAVEFORMATEX& wfex, BYTE*& buffer, uint32_t& length);

	// Fetches format of file with given id.
	bool FetchFormat(const uint16_t id, WAVEFORMATEX& wfex) const;

	// Releases sound file with given id.
	bool Release(const uint16_t id);

	// Returns TRUE if music file with given id can be streamed.
	bool CanStream(const uint16_t id) const;

	// Prepares music file for streaming and returns format of file.
	bool StartStreaming(uint16_t id, WAVEFORMATEX &wfex);

	// Finishes streaming of music file with given id.
	bool StopStreaming(uint16_t id);

	// Tries to fill 'count' bytes of streaming file with given id to buffer 'spBuffer',
	// Returns number of actually written bytes.
	uint32_t GetStreamData(uint16_t id, std::unique_ptr<BYTE[]>& spBuffer, const uint32_t count);

private:

	bool _can_fetch(uint16_t id) const;
	bool _can_stream(uint16_t id) const;
};

