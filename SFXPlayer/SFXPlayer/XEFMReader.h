#pragma once

#include <string>
#include <fstream>
#include <shared_mutex>

class XEFMReader
{
private:

	std::wstring  m_FileName; // file properties
	uint32_t      m_MinPos;
	uint32_t      m_MaxPos;
	uint32_t      m_CurPos;

	// Mutex for thread safety.
	mutable std::shared_mutex m_Lock;

	// Assosiated stream.
	std::ifstream m_Stream;

public:

	enum class XSEEK_TYPE
	{
		XSEEK_BEGIN,
		XSEEK_CURRENT,
		XSEEK_END
	};

	XEFMReader();

	void Open(const wchar_t* pFileName);

	void Close();

	bool IsOpen() const; 

	bool IsEOF() const;

	bool ReadLine(std::string& s);

	uint32_t ReadBytes(char* pBuffer, const uint32_t nCount);

	uint32_t Tell() const;

	bool Seek(const uint32_t nPos, XSEEK_TYPE seektype = XSEEK_TYPE::XSEEK_BEGIN);

	uint32_t GetSize() const;
};

