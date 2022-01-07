#include "XSoundFile.h"
#include "XAux.h"
#include "XException.h"
#include "XEFMReader.h"

XSoundFile::XSoundFile()
{
	m_bAssigned = false;
	m_bLoaded = false;
	m_nRefCount = 0;
}

void XSoundFile::AssignFile(const wchar_t* pFileName, uint16_t id)
{
	std::unique_lock lock{ m_Lock };

	if (m_bAssigned)
	{
		// Can't assign twice:
		throw XException(L"XSoundFile()::AssignFile(): file is already assigned");
	}

	// Try to open as Windows Media file:

	try
	{
		OpenWMFile(pFileName);

		// OK. Saving file type:

		m_FileType = XSoundFileType::XSoundFile_WMA;

	}
	catch (const XException& e1)
	{
		// Fail. Try to open as plain WAV.
		try
		{
			OpenWAVFile(pFileName);

			// OK. Let it be.

			m_FileType = XSoundFileType::XSoundFile_WAV;

		}
		catch (const XException& e2)
		{
			// Fail again. Leaving.

			std::wstring error = std::wstring(e1.GetError()) + L" && " + std::wstring(e2.GetError());
			XException ex(error.c_str());

			throw XException(ex, L"XSoundFile::AssignFile(): can't load file '%s'", pFileName);

		}
	}

	m_bAssigned = true;
	m_bLoaded = false;
	m_nSoundId = id;
	m_strFileName = pFileName;

}

bool XSoundFile::Load()
{
	std::unique_lock lock{ m_Lock };

	if (!m_bAssigned)
	{
		// Not assigned. Nothing to load.
		return false;
	}

	if (m_bLoaded)
	{
		// Already loaded.
		return true;
	}

	// Try to load:
	try
	{
		switch (m_FileType)
		{
    		case XSoundFileType::XSoundFile_WMA:
	    	{
				OpenWMFile(m_strFileName.data(), true);

			    break;
		    }
		    case XSoundFileType::XSoundFile_WAV:
		    {
				OpenWAVFile(m_strFileName.data(), true);
 
				break;
    		}
		    default:
			{
				// Odd.
				return false;

				break;
		    }
		}
	}
	catch (const XException& e)
	{
		// Failed.

		UNREFERENCED_PARAMETER(e);

		return false;
	}


	// Everything is OK.

	m_bLoaded = true;

	return true;
}

bool XSoundFile::IsLoaded() const
{
	std::shared_lock lock{ m_Lock };

	return ( m_bLoaded && m_bAssigned);
}

bool XSoundFile::Unload()
{
	std::unique_lock lock{ m_Lock };

	if ( (!m_bAssigned) || (!m_bLoaded) || (m_nRefCount)) 
	{
		return false;
	}

	m_spSamples.reset(nullptr);

	m_bLoaded = false;

	return true;
}

uint16_t XSoundFile::GetId() const
{
	std::shared_lock lock{ m_Lock };

	return m_nSoundId;
}

uint32_t XSoundFile::GetSize() const
{
	std::shared_lock lock{ m_Lock };

	return m_nFileSize;
}

bool XSoundFile::GetFormat(WAVEFORMATEX& wfex) const
{
	std::shared_lock lock{ m_Lock };

	if (!m_bAssigned)
	{
		// Nothing to return.
		return false;
	}

	wfex = m_SoundFormat;

	return true;
}

bool XSoundFile::GetData(std::unique_ptr<BYTE[]>& spdata) const
{
	std::shared_lock lock{ m_Lock };

	if ((!m_bAssigned) || (!m_bLoaded))
	{
		// Nothing to return.
		return false;
	}

	// Return copy of samples.

	spdata = std::make_unique<BYTE[]>(m_nFileSize);

	std::copy(m_spSamples.get(), m_spSamples.get() + m_nFileSize, spdata.get());

	return true;
}

bool XSoundFile::GetDataDirect(BYTE*& refBuffer)
{
	std::unique_lock lock{ m_Lock };

	if ((!m_bAssigned) || (!m_bLoaded))
	{
		// Nothing to return.
		return false;
	}

	// Return direct pointer to samples.

	refBuffer = m_spSamples.get();
	m_nRefCount++;

	return true;
}

void XSoundFile::FreeData()
{
	std::unique_lock lock{ m_Lock };

	if (m_nRefCount <= 0)
	{
		throw XException(L"XSoundFile::FreeData(): internal counter is zero");
	}

	m_nRefCount--;
}

void XSoundFile::OpenWMFile(const wchar_t* pFileNameSource, bool bLoad)
{
	XEFMReader reader;

	// Try to open file and obtain it's length:
	try
	{
		reader.Open(pFileNameSource);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XSoundFile()::OpenWMFile(): can't open file '%s'", pFileNameSource);
	}

	std::uint32_t FileSize = reader.GetSize();

	// Allocate memory for reading:
	HGLOBAL hGH = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, FileSize);
	if (!hGH)
	{
		throw XException(L"XSoundFile()::OpenWMFile(): memory allocation error");
	};

	std::unique_ptr<HGLOBAL, decltype(&XAux::HGLOBAL_deleter)> spGH{ reinterpret_cast<HGLOBAL*>(hGH), XAux::HGLOBAL_deleter };

	LPVOID lpBuffer = GlobalLock(hGH);
	if (!lpBuffer)
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't lock memory");
	};

	// Read file contents and unlock memory:
	uint32_t bytes_read = reader.ReadBytes(static_cast<char*>(lpBuffer), FileSize);

	reader.Close();

	GlobalUnlock(hGH);

	// Now we have to construct IStream object from file in memory:

	IStream* pStream = NULL;

	if (CreateStreamOnHGlobal(hGH, TRUE, &pStream) != S_OK)
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't create IStream object");
	};

	std::unique_ptr<IStream, decltype(&XAux::COM_deleter)> spStream{ pStream, XAux::COM_deleter };

	// Now we have to construct IWMSyncReader object to read from the stream:

	IWMSyncReader* pSyncReader;

	if FAILED(WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &pSyncReader))
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't create IWMSyncReader object");
	};

	std::unique_ptr<IWMSyncReader, decltype(&XAux::COM_deleter)> spSyncReader{ pSyncReader, XAux::COM_deleter };

	// Open our stream with the reader and declare with SetReadStreamSamples(0, FALSE) that we want to get uncompressed data: 

	if FAILED(pSyncReader->OpenStream(pStream))
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't open stream for reading");
	};

	pSyncReader->SetReadStreamSamples(0, FALSE);

	// Get media properties:

	IWMOutputMediaProps* pMediaProps = NULL;
	DWORD                dwPropsBufferSize;
	WM_MEDIA_TYPE*       pMediaType = NULL;

	if FAILED(pSyncReader->GetOutputProps(0, &pMediaProps))
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't get stream properties");
	};

	std::unique_ptr<IWMOutputMediaProps, decltype(&XAux::COM_deleter)> spMediaProps{ pMediaProps, XAux::COM_deleter };

	pMediaProps->GetMediaType(NULL, &dwPropsBufferSize);

	std::unique_ptr<BYTE[]> spPropsData{ new BYTE[dwPropsBufferSize] };

	if FAILED(pMediaProps->GetMediaType((WM_MEDIA_TYPE*)spPropsData.get(), &dwPropsBufferSize))
	{
		throw XException(L"XSoundFile()::OpenWMFile(): can't get media type");
	};

	pMediaType = (WM_MEDIA_TYPE*)spPropsData.get();

	// Make sure that our file is audio:

	if ((pMediaType->majortype != WMMEDIATYPE_Audio) || (pMediaType->formattype != WMFORMAT_WaveFormatEx))
	{
		throw XException(L"XSoundFile()::OpenWMFile(): unsupported file type");
	};

	// Save file format:
	WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX*)pMediaType->pbFormat;

	m_SoundFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_SoundFormat.nChannels = pWaveFormatEx->nChannels;
	m_SoundFormat.nSamplesPerSec = pWaveFormatEx->nSamplesPerSec;
	m_SoundFormat.wBitsPerSample = pWaveFormatEx->wBitsPerSample;
	m_SoundFormat.nBlockAlign = m_SoundFormat.nChannels * m_SoundFormat.wBitsPerSample / 8;
	m_SoundFormat.nAvgBytesPerSec = m_SoundFormat.nSamplesPerSec * m_SoundFormat.nBlockAlign;
	m_SoundFormat.cbSize = 0;

	// Unpacking:

	std::list < std::unique_ptr<INSSBuffer, decltype(&XAux::COM_deleter)>> listBuffers;
	INSSBuffer *pBuffer = NULL;

	m_nFileSize = 0;

	for (;;)
	{
		HRESULT hResult;
		QWORD   qwSampleTime;
		QWORD   qwSampleDuration;
		DWORD   dwFlags;
		DWORD   dwOutputNum;
		WORD    wStreamNum;
		DWORD   dwSamplesLength;

		hResult = pSyncReader->GetNextSample(0, &pBuffer, &qwSampleTime, &qwSampleDuration, &dwFlags, &dwOutputNum, &wStreamNum);

		if (hResult == NS_E_NO_MORE_SAMPLES)
		{
			break;
		}
		else
			if (hResult != S_OK)
			{
				throw XException(L"XSoundFile()::OpenWMFile(): unpacking error");

				break;
			};

		pBuffer->GetLength(&dwSamplesLength);

		m_nFileSize += dwSamplesLength;

		if (bLoad)
		{
			// Store buffer in the list:
			listBuffers.emplace_back(pBuffer, XAux::COM_deleter);
		}
		else
		{
			// Don't store - we are not loading.
			pBuffer->Release();
		}
	};

	if (!bLoad)
	{
		// All is done. We got format & file size. Leaving.
		return;
	}

	// Loading. Copy all buffers into one:

	m_spSamples.reset(new BYTE[m_nFileSize]);

	DWORD dwOffset{ 0 };

	for (const auto &ptr_buf : listBuffers)
	{
		DWORD dwLength;
		BYTE* pData;

		ptr_buf->GetBufferAndLength(&pData, &dwLength);

		std::copy(pData, pData + dwLength, m_spSamples.get() + dwOffset);
		dwOffset += dwLength;
	}

};

void XSoundFile::OpenWAVFile(const wchar_t* pFileNameSource, bool bLoad)
{
	XEFMReader reader;

	// Try to open file:
	try
	{
		reader.Open(pFileNameSource);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XSoundFile()::OpenWAVFile(): can't open file '%s'", pFileNameSource);

	}

	// WAVE chunk's IDs:
	const DWORD WAV_RIFF = 0x46464952;
	const DWORD WAV_WAVE = 0x45564157;
	const DWORD WAV_fmt  = 0x20746D66;
	const DWORD WAV_data = 0x61746164;

	// WAVE chunk's header:
	struct WAV_CHUNK
	{
		DWORD chkId;   // chunk id
		DWORD chkSize; // chunk size
	};

	WAV_CHUNK WaveChk;
	DWORD     dwSign, dwRiffSize;
	DWORD     dwFormatOffset = 0, dwDataOffset = 0, dwFormatLength = 0, dwDataLength = 0;

	// Try to read first chunk - RIFF:

	reader.ReadBytes(reinterpret_cast<char*>(&WaveChk), sizeof(WAV_CHUNK));

	if (WaveChk.chkId != WAV_RIFF)
	{
		throw XException(L"XSoundFile()::OpenWAVFile(): unsupported file type");
	};

	dwRiffSize = WaveChk.chkSize + sizeof(WAV_CHUNK);

	// Read WAVE-form signature:

	reader.ReadBytes(reinterpret_cast<char*>(&dwSign), sizeof(DWORD));
	
	if (dwSign != WAV_WAVE)
	{
		throw XException(L"XSoundFile()::OpenWAVFile(): unsupported file type");
	};

	// Iterate through all chunks until end of file:

	do
	{
		reader.ReadBytes(reinterpret_cast<char*>(&WaveChk), sizeof(WAV_CHUNK));

		switch (WaveChk.chkId)
		{
    	   	case WAV_fmt:
		    {
    			// data chunk (format description):
	    		dwFormatOffset = reader.Tell();
		    	dwFormatLength = WaveChk.chkSize;

    			break;
		    };
		    case WAV_data:
		    {
    			// samples chunk:
	    		dwDataOffset = reader.Tell();
		    	dwDataLength = WaveChk.chkSize;

    			break;
		    };

		};

		// "Jump over" current chunk...
    } while (reader.Seek(WaveChk.chkSize, XEFMReader::XSEEK_TYPE::XSEEK_CURRENT));

	if (!dwFormatOffset || !dwDataOffset)
	{
		// Required chunks weren't read:
		throw XException(L"XSoundFile()::OpenWAVFile(): format type chunk and/or wave data not detected");
	};

	// Now read file format into WAVEFORMATEX structure:
	std::unique_ptr<BYTE[]> spFormat(new BYTE[dwFormatLength]);

	LPWAVEFORMATEX lpFormat = reinterpret_cast<LPWAVEFORMATEX>(spFormat.get());

	reader.Seek(dwFormatOffset);
	reader.ReadBytes(reinterpret_cast<char*>(lpFormat), dwFormatLength);

	// Checking file format (must be uncompressed) & storing:
	if (lpFormat->wFormatTag != WAVE_FORMAT_PCM)
	{
		// Format is not plain uncompressed PCM:
		throw XException(L"XSoundFile()::OpenWAVFile(): file must be uncompressed");
	}

	m_SoundFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_SoundFormat.nChannels = lpFormat->nChannels;
	m_SoundFormat.nSamplesPerSec = lpFormat->nSamplesPerSec;
	m_SoundFormat.wBitsPerSample = lpFormat->wBitsPerSample;
	m_SoundFormat.nBlockAlign = m_SoundFormat.nChannels * m_SoundFormat.wBitsPerSample / 8;
	m_SoundFormat.nAvgBytesPerSec = m_SoundFormat.nSamplesPerSec * m_SoundFormat.nBlockAlign;
	m_SoundFormat.cbSize = 0;

	m_nFileSize = dwDataLength;

	if (!bLoad)
	{
		// No need to load: we got size & format. Leaving.
		return;
	};

	// Read samples:
	m_spSamples.reset(new BYTE[dwDataLength]);

	bool bOK = reader.Seek(dwDataOffset);
	uint32_t bytes_read = reader.ReadBytes(reinterpret_cast<char*>(m_spSamples.get()), dwDataLength);
};
