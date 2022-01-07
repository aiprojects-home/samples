#include <fstream>
#include <locale>
#include <list>
#include <limits>
#include <mutex>

#include "framework.h"
#include "XSoundBank.h"
#include "XAux.h"
#include "XEFMReader.h"

XSoundBank::XSoundBankParser::XSoundBankParser(const XSoundBank *pOwner)
{
	m_pOwner = pOwner;

	XStringParser::Init(
	{
		XParserElement(std::wregex(L"\\s*<soundbank>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr, std::wregex(L"\\s*</soundbank>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr,
		{
			XParserElement(std::wregex(L"\\s*<sfx>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr, std::wregex(L"\\s*</sfx>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr,
				{
					XParserElement(std::wregex(L"\\s*<file \\s*name\\s*=\\s*\\\"(.*)\\\" \\s*id\\s*=\\s*\\\"(\\d*)\\\"\\s*\\/>", std::regex_constants::ECMAScript | std::regex_constants::icase), &SFX_FILE_Handler)
				}),
			XParserElement(std::wregex(L"\\s*<music>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr, std::wregex(L"\\s*</music>", std::regex_constants::ECMAScript | std::regex_constants::icase), nullptr,
				{
					XParserElement(std::wregex(L"\\s*<file \\s*name\\s*=\\s*\\\"(.*)\\\" \\s*id\\s*=\\s*\\\"(\\d*)\\\"\\s*\\/>", std::regex_constants::ECMAScript | std::regex_constants::icase), &MUSIC_FILE_Handler)
				})
		})
	});

}

void XSoundBank::XSoundBankParser::Parse(const wchar_t* pFileName)
{
	XEFMReader reader;

	try
	{
		reader.Open(pFileName);
	}
	catch (const XException& e)
	{
		// Can't open bank file.
		throw XException(e, L"XSoundBank::XSoundBankParser::Parse(): can't open file");
	};

	std::string line;
	std::list<std::wstring> listStrings;

	// Reading file lines and puting them into vector.
	while (!reader.IsEOF())
	{
		reader.ReadLine(line);

		listStrings.emplace_back(line.begin(), line.end());

	};

	// Iterating data in the vector via parser.

	m_SFXCol.clear();
	m_MusicCol.clear();

	try
	{
		XStringParser::Parse(listStrings.begin(), listStrings.end());
	}
	catch (const XException& e)
	{
		throw XException(e, L"XSoundBank::XSoundBankParser::Parse(): parser error");
	}

}

bool XSoundBank::XSoundBankParser::SFX_FILE_Handler(const XStringParser& obj, const std::wsmatch& match)
{
	XSoundBankParser& p = (XSoundBankParser&)const_cast<XStringParser&>(obj);

	std::wstring strFileName = match[1];
	std::wstring strId = match[2];

	unsigned long id;
	
	try
	{
		id = std::stoul(strId);
	}
	catch(const std::exception& e)
	{
		UNREFERENCED_PARAMETER(e);

		throw XException(L"XSoundBank::XSoundBankParser::SFX_FILE_Handler(): can't convert 'id' value");
	}

	if (id > 0xFFFF)
	{
		throw XException(L"XSoundBank::XSoundBankParser::SFX_FILE_Handler(): 'id' value is out of range");
	}

	if (p.m_SFXCol.find(uint16_t(id)) != p.m_SFXCol.end())
	{
		throw XException(L"XSoundBank::XSoundBankParser::SFX_FILE_Handler(): 'id' value is already in use");
	}

	// OK. Storing file name.

	p.m_SFXCol[uint16_t(id)] = strFileName;

	return true;
}

bool XSoundBank::XSoundBankParser::MUSIC_FILE_Handler(const XStringParser& obj, const std::wsmatch& match)
{
	XSoundBankParser& p = (XSoundBankParser&)const_cast<XStringParser&>(obj);

	std::wstring strFileName = match[1];
	std::wstring strId = match[2];

	unsigned long id;

	try
	{
		id = std::stoul(strId);
	}
	catch (const std::exception& e)
	{
		UNREFERENCED_PARAMETER(e);

		throw XException(L"XSoundBank::XSoundBankParser::MUSIC_FILE_Handler(): can't convert 'id' value");
	}

	if (id > 0xFFFF)
	{
		throw XException(L"XSoundBank::XSoundBankParser::MUSIC_FILE_Handler(): 'id' value is out of range");
	}

	if (p.m_MusicCol.find(uint16_t(id)) != p.m_MusicCol.end())
	{
		throw XException(L"XSoundBank::XSoundBankParser::MUSIC_FILE_Handler(): 'id' value is already in use");
	}

	// OK. Storing file name.

	p.m_MusicCol[uint16_t(id)] = strFileName;

	return true;
}

XSoundBank::XSoundBank(const uint32_t maxsize) : m_Parser(this), m_MaxBankSize{ maxsize }
{
	m_bAssigned = false;
	m_CurrentSize = 0;
}

void XSoundBank::AssignFile(const wchar_t* pFileName)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// Can't assign twice.
		throw XException(L"XSoundBank()::AssignFile(): file is already assigned");
	}

	try
	{
		m_Parser.Parse(pFileName);
	}
	catch (const XException& e)
	{
		// Error while parsing.
		throw XException(e, L"XSoundBank::AssignFile(): invalid data in file '%s'", pFileName);
	}

	// Everything is OK. File is parsed.

	// Checking sounds.
	std::list<std::unique_ptr<XSoundFile>> FileList;
	std::unique_ptr<XSoundFile>            spXSoundFile;

	for (auto &v : m_Parser.m_SFXCol)
	{
		// Trying to assign every file.
		spXSoundFile = std::make_unique<XSoundFile>();

		try
		{
			spXSoundFile->AssignFile(v.second.c_str(), v.first);
		}
		catch (const XException& e)
		{
			throw XException(e, L"XSoundBank::AssignFile(): can't assign sound file '%s'", v.second.c_str());
		}

		FileList.push_back(std::move(spXSoundFile));
	}

	// Checking music.
	std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>> MusicTable;
	std::unique_ptr<XMusicFile>                               spXMusicFile;

	for (auto &v : m_Parser.m_MusicCol)
	{
		// Trying to assign every file.
		spXMusicFile = std::make_unique<XMusicFile>();

		try
		{
			spXMusicFile->AssignFile(v.second.c_str(), v.first);
		}
		catch (const XException& e)
		{
			throw XException(e, L"XSoundBank::AssignFile(): can't assign music file '%s'", v.second.c_str());
		}

		MusicTable[v.first] = std::move(spXMusicFile);
	}

	// Destroying all previous data.

	m_XSoundFileList.clear();
	m_XSoundFilePos.clear();
	m_XMusicFileTable.clear();

	// All files are assigned. Copying them into internal list.

	// Copy sounds.
	m_XSoundFileList = std::move(FileList);

	// Saving positions of objects in hash-table:
	for (auto it = m_XSoundFileList.begin(); it != m_XSoundFileList.end(); ++it)
	{
		m_XSoundFilePos[it->get()->GetId()] = it;
	}

	// Copy music.
	m_XMusicFileTable = std::move(MusicTable);

	// OK

	m_bAssigned = true;
	m_strFileName = pFileName;
}

bool XSoundBank::CanFetch(const uint16_t id) const
{
	std::shared_lock lock(m_Lock);

	return _can_fetch(id);

}

bool XSoundBank::Fetch(const uint16_t id, WAVEFORMATEX& wfex, BYTE*& buffer, uint32_t& length)
{
	std::unique_lock lock(m_Lock);

	if (!_can_fetch(id))
	{
		return false;
	}

	std::list<std::unique_ptr<XSoundFile>>::iterator it = m_XSoundFilePos.at(id);

	// Moving required buffer to top of the list:
	auto ptr = std::move(*it);

	m_XSoundFileList.erase(it);
	m_XSoundFileList.push_front(std::move(ptr));
	m_XSoundFilePos[id] = m_XSoundFileList.begin();

	// Making sure that sound is loaded - unload all buffers if not enough memory.

	uint32_t req_bytes = m_XSoundFileList.front()->GetSize();

	if (m_XSoundFileList.front()->IsLoaded())
	{
		// Skip loading:
		goto skip_load;
	}

	// Unloading procedure.
	for (auto rev_it = m_XSoundFileList.rbegin(); rev_it != m_XSoundFileList.rend(); ++rev_it)
	{
		if ((req_bytes + m_CurrentSize) > m_MaxBankSize)
		{
			if (!rev_it->get()->IsLoaded())
			{
				// Already unloaded.
				continue;
			};

			// Unload another one.
			if (!rev_it->get()->Unload())
			{
				// Fail. Can't unload (reference counter is not zero?) & don't have memory.
				continue;
			};

			m_CurrentSize -= rev_it->get()->GetSize();
		}
		else
		{
			// We have enough memory.
			break;
		}
	}

	// Check: Do we have enough?
	if ((req_bytes + m_CurrentSize) > m_MaxBankSize)
	{
		// Still don't have enough.
    	return false;
	}

	if (!m_XSoundFileList.front()->Load())
	{
		// Something happened. Can't load.
		return false;
	}

	// Adjust memory status.
	m_CurrentSize += m_XSoundFileList.front()->GetSize();

skip_load:

	// Return format & samples.

	m_XSoundFileList.front()->GetFormat(wfex);
	m_XSoundFileList.front()->GetDataDirect(buffer);
	length = m_XSoundFileList.front()->GetSize();

	return true;
}

bool XSoundBank::FetchFormat(const uint16_t id, WAVEFORMATEX& wfex) const
{
	std::shared_lock lock(m_Lock);

	if (!_can_fetch(id))
	{
		return false;
	}

	std::list<std::unique_ptr<XSoundFile>>::iterator it = m_XSoundFilePos.at(id);

	it->get()->GetFormat(wfex);

	return true;
}

bool XSoundBank::Release(const uint16_t id)
{
	std::list<std::unique_ptr<XSoundFile>>::iterator it;

	try
	{
		it = m_XSoundFilePos.at(id);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// No such value.
		return false;
	}

	it->get()->FreeData();

	return true;
}

bool XSoundBank::CanStream(const uint16_t id) const
{
	std::shared_lock lock(m_Lock);

	return _can_stream(id);
}

bool XSoundBank::StartStreaming(uint16_t id, WAVEFORMATEX &wfex)
{
	std::unique_lock lock(m_Lock);

	if (!_can_stream(id))
	{
		return false;
	}

	const std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>>::mapped_type &v = m_XMusicFileTable.at(id);

	return v->DecodeStart(wfex);
}

bool XSoundBank::StopStreaming(uint16_t id)
{
	std::unique_lock lock(m_Lock);

	if (!_can_stream(id))
	{
		return false;
	}

	const std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>>::mapped_type &v = m_XMusicFileTable.at(id);

	return v->DecodeStop();
}

uint32_t XSoundBank::GetStreamData(uint16_t id, std::unique_ptr<BYTE[]>& spBuffer, uint32_t count)
{
	std::unique_lock lock(m_Lock);

	if (!_can_stream(id))
	{
		return 0;
	}

	const std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>>::mapped_type &v = m_XMusicFileTable.at(id);

	if (!v->IsDecoding())
	{
		return 0;
	}

	return v->DecodeBytes(spBuffer, count);
}


bool XSoundBank::_can_fetch(uint16_t id) const
{
	std::list<std::unique_ptr<XSoundFile>>::iterator it;

	try
	{
		it = m_XSoundFilePos.at(id);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// No such value.
		return false;
	}

	return true;
}

bool XSoundBank::_can_stream(uint16_t id) const
{
	try
	{
		const std::unordered_map<uint16_t, std::unique_ptr<XMusicFile>>::mapped_type &v = m_XMusicFileTable.at(id);
	}
	catch (const std::out_of_range& e)
	{
		UNREFERENCED_PARAMETER(e);

		// No such value.
		return false;
	}

	return true;
}
