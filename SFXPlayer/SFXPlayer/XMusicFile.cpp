#include "XMusicFile.h"

#include "XEFM.h"
#include "XEFMReader.h"
#include "XException.h"
#include "XAux.h"

XMusicFile::XMusicFile() : m_spBuffer{ nullptr, XAux::COM_deleter }, m_spWMReader{ nullptr, XAux::COM_deleter },
                           m_spStream{ nullptr, XAux::COM_deleter }
{
	m_bAssigned = false;
}

XMusicFile::~XMusicFile()
{
	if (m_bDecoding)
	{
		DecodeStop();
	}
}

void XMusicFile::AssignFile(const wchar_t* pFileName, const uint16_t id)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// Can't assign twice:
		throw XException(L"XMusicFile()::AssignFile(): file is already assigned");
	}

	XEFMReader reader;

	try
	{
		reader.Open(pFileName);
	}
	catch (const XException& e)
	{
		// File doesn't exist.
		throw XException(e, L"XMusicFile()::AssignFile(): can't open file '%s'", pFileName);
	}

	m_bAssigned = true;
	m_nMusicId = id;
	m_bDecoding = false;
	m_strFileName = pFileName;
	m_nFileSize = reader.GetSize();

	reader.Close();
}

bool XMusicFile::DecodeStart(WAVEFORMATEX &wfex)
{
	std::unique_lock lock{ m_Lock };
	
	if (m_bDecoding)
	{
		throw XException(L"XMusicFile()::DecodeStart(): file '%s' is already decoding", m_strFileName);
	}

	XEFMReader reader;

	// Try to open music file.
	try
	{
		reader.Open(m_strFileName.c_str());
	}
	catch (const XException& e)
	{
		throw XException(e, L"XMusicFile()::DecodeStart(): can't open file '%s'", m_strFileName);
	}

	// Allocate memory for reading:
	HGLOBAL hGH = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, m_nFileSize);
	if (!hGH)
	{
		throw XException(L"XMusicFile()::DecodeStart() : memory allocation error");
	};

	std::unique_ptr<HGLOBAL, XAux::HGLOBAL_deleter_type> spGH{ reinterpret_cast<HGLOBAL*>(hGH), XAux::HGLOBAL_deleter };

	LPVOID lpBuffer = GlobalLock(hGH);
	if (!lpBuffer)
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't lock memory");
	};

	// Read file contents and unlock memory:
	uint32_t bytes_read = reader.ReadBytes(static_cast<char*>(lpBuffer), m_nFileSize);

	reader.Close();

	GlobalUnlock(hGH);

	// Now we have to construct IStream object from file in memory:

	IStream* pStream = NULL;

	if (CreateStreamOnHGlobal(hGH, TRUE, &pStream) != S_OK)
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't create IStream object");
	};

	std::unique_ptr<IStream, XAux::COM_deleter_type> spStream{ pStream, XAux::COM_deleter };

	// Now we have to construct IWMSyncReader object to read from the stream:

	IWMSyncReader* pSyncReader;

	if FAILED(WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &pSyncReader))
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't create IWMSyncReader object");
	};

	std::unique_ptr<IWMSyncReader, XAux::COM_deleter_type> spSyncReader{ pSyncReader, XAux::COM_deleter };

	// Open our stream with the reader and declare with SetReadStreamSamples(0, FALSE) cause we want to get uncompressed data: 

	if FAILED(pSyncReader->OpenStream(pStream))
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't open stream for reading");
	};

	pSyncReader->SetReadStreamSamples(0, FALSE);

	// Get media properties:

	IWMOutputMediaProps* pMediaProps = NULL;
	DWORD                dwPropsBufferSize;
	WM_MEDIA_TYPE*       pMediaType = NULL;

	if FAILED(pSyncReader->GetOutputProps(0, &pMediaProps))
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't get stream properties");
	};

	std::unique_ptr<IWMOutputMediaProps, XAux::COM_deleter_type> spMediaProps{ pMediaProps, XAux::COM_deleter };

	pMediaProps->GetMediaType(NULL, &dwPropsBufferSize);

	std::unique_ptr<BYTE[]> spPropsData{ new BYTE[dwPropsBufferSize] };

	if FAILED(pMediaProps->GetMediaType((WM_MEDIA_TYPE*)spPropsData.get(), &dwPropsBufferSize))
	{
		throw XException(L"XMusicFile()::DecodeStart(): can't get media type");
	};

	pMediaType = (WM_MEDIA_TYPE*)spPropsData.get();

	// Make sure that our file is audio:

	if ((pMediaType->majortype != WMMEDIATYPE_Audio) || (pMediaType->formattype != WMFORMAT_WaveFormatEx))
	{
		throw XException(L"XMusicFile()::DecodeStart(): unsupported file type");
	};

	// Save file format and data for decoding:
	WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX*)pMediaType->pbFormat;

	m_MusicFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_MusicFormat.nChannels = pWaveFormatEx->nChannels;
	m_MusicFormat.nSamplesPerSec = pWaveFormatEx->nSamplesPerSec;
	m_MusicFormat.wBitsPerSample = pWaveFormatEx->wBitsPerSample;
	m_MusicFormat.nBlockAlign = m_MusicFormat.nChannels * m_MusicFormat.wBitsPerSample / 8;
	m_MusicFormat.nAvgBytesPerSec = m_MusicFormat.nSamplesPerSec * m_MusicFormat.nBlockAlign;
	m_MusicFormat.cbSize = 0;

	wfex = m_MusicFormat;

	m_spBuffer = nullptr;
	m_spWMReader = std::move(spSyncReader);
	m_spStream = std::move(spStream);
	
	spGH.release();

	m_bDecoding = true;

	return true;
}

uint32_t XMusicFile::DecodeBytes(std::unique_ptr<uint8_t[]>& spDestBuffer, const uint32_t count)
{
	std::unique_lock lock{ m_Lock };

	if (!m_bDecoding)
	{
		return 0;
	}

	uint32_t nBytesUnpacked{ 0 };

	// If we have samples in buffer from previous call -- store them.
	if (m_spBuffer != nullptr)
	{
		uint8_t *ptr;

		m_spBuffer->GetBuffer(&ptr);

		std::copy(ptr + m_nRemainOffset, ptr + m_nRemainOffset + m_nBytesRemain, spDestBuffer.get());
		nBytesUnpacked += m_nBytesRemain;

		m_spBuffer = nullptr;
	}

	// Unpack until we'll get full buffer or reach the end.
	while (nBytesUnpacked < count)
	{
		HRESULT     hResult;
		QWORD       qwSampleTime;
		QWORD       qwSampleDuration;
		DWORD       dwFlags;
		DWORD       dwOutputNum;
		WORD        wStreamNum;
		DWORD       dwSamplesLength;
		INSSBuffer* pBuffer;
		BYTE*       pSource;

		hResult = m_spWMReader->GetNextSample(0, &pBuffer, &qwSampleTime, &qwSampleDuration, &dwFlags, &dwOutputNum, &wStreamNum);

		if (hResult == NS_E_NO_MORE_SAMPLES)
		{
			break;
		}
		else
		if (hResult != S_OK)
		{
			throw XException(L"XMusicFile()::DecodeBytes(): unpacking error");

			break;
		};

		m_spBuffer.reset(pBuffer);

		// Storing another part of samples...

		m_spBuffer->GetLength(&dwSamplesLength);
		m_spBuffer->GetBuffer(&pSource);

		// We can't copy more bytes than can be stored in the buffer.
		uint32_t nBytesToCopy{ min(dwSamplesLength, count - nBytesUnpacked) };

		std::copy(pSource, pSource + nBytesToCopy, spDestBuffer.get() + nBytesUnpacked);

		nBytesUnpacked += nBytesToCopy;

		m_nBytesRemain = dwSamplesLength - nBytesToCopy;
		m_nRemainOffset = nBytesToCopy;

		if (m_nBytesRemain == 0)
		{
			// Buffer was entirely copied.
			m_spBuffer = nullptr;
		};

	}

	return nBytesUnpacked;
}

bool XMusicFile::DecodeStop()
{
	std::unique_lock lock{ m_Lock };

	if (!m_bDecoding)
	{
		return false;
	}

	if (m_spWMReader != nullptr)
	{
		m_spWMReader->Close();
	};

	m_spBuffer = nullptr;
	m_spWMReader = nullptr;
	m_spStream = nullptr;

	m_bDecoding = false;

	return true;
}

bool XMusicFile::IsDecoding() const
{
	std::unique_lock lock{ m_Lock };

	return m_bDecoding;
}
