#include "pch.h"
#include "XWMADecoder.h"
#include "XException.h"
#include "XEFMReader.h"

XWMADecoder::XWMADecoder() : m_upBuffer { nullptr, XAux::COM_deleter }, m_upWMReader{ nullptr, XAux::COM_deleter },
m_upStream{ nullptr, XAux::COM_deleter }
{
	_ResetInternalData();
}

XWMADecoder::~XWMADecoder()
{

}

XSoundDecoder* XWMADecoder::CreateStatic()
{
	return new XWMADecoder();
}

void XWMADecoder::_OpenFile(const wchar_t* pFileName, uint8_t Mode)
{
	XEFMReader reader;

	// 1. ��������� ����� OPEN_GENERIC (=0, ������� ������)
	
	// �������� ������ ���� � �������� ��� �����:
	try
	{
		reader.Open(pFileName);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XWMADecoder()::_OpenFile(): can't open file '%s'", pFileName);
	}

	std::uint32_t nFileSize = reader.GetSize();

	// �������� ������ ��� ������:
	HGLOBAL hGH = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, nFileSize);
	if (!hGH)
	{
		throw XException(L"XWMADecoder()::_OpenFile(): memory allocation error");
	};

	std::unique_ptr<HGLOBAL, decltype(&XAux::HGLOBAL_deleter)> upGH{ reinterpret_cast<HGLOBAL*>(hGH), XAux::HGLOBAL_deleter };

	LPVOID lpBuffer = GlobalLock(hGH);
	if (!lpBuffer)
	{
		throw XException(L"XWMADecoder()::_OpenFile(): can't lock memory");
	};

	// ������ ���������� ����� � ������������ ������.
	uint32_t nBytesRead = reader.ReadBytes(static_cast<char*>(lpBuffer), nFileSize);

	reader.Close();

	GlobalUnlock(hGH);

	// ������ ����� ������� ����� IStream �� ����� � ������:

	IStream* pStream = NULL;

	if (CreateStreamOnHGlobal(hGH, TRUE, &pStream) != S_OK)
	{
		throw XException(L"XWMADecoder()::_OpenFile(): can't create IStream object");
	};

	std::unique_ptr<IStream, decltype(&XAux::COM_deleter)> upStream{ pStream, XAux::COM_deleter };

	// ������ ����� ������� IWMSyncReader �������� ����� IStream:

	IWMSyncReader* pSyncReader = NULL;

	if FAILED(WMCreateSyncReader(NULL, WMT_RIGHT_PLAYBACK, &pSyncReader))
	{
		throw XException(L"XWMADecoder()::_OpenFile(): can't create IWMSyncReader object");
	};

	std::unique_ptr<IWMSyncReader, decltype(&XAux::COM_deleter)> upSyncReader{ pSyncReader, XAux::COM_deleter };

	// ��������� ��� ����� � ������ � �������� SetReadStreamSamples(0, FALSE) ����� ������ ������������� ������:

	if FAILED(pSyncReader->OpenStream(pStream))
	{
		throw XException(L"XWMADecoder()::_OpenFile(): can't open stream for reading");
	};

	pSyncReader->SetReadStreamSamples(0, FALSE);

	// 2. ��������� ����� OPEN_READ_FORMAT

	if (Mode & (uint8_t)OpenMode::OPEN_READ_FORMAT)
	{
		IWMOutputMediaProps* pMediaProps = NULL;
		DWORD                dwPropsBufferSize = 0;
		WM_MEDIA_TYPE*       pMediaType = NULL;

		if FAILED(pSyncReader->GetOutputProps(0, &pMediaProps))
		{
			throw XException(L"XWMADecoder()::_OpenFile(): can't get stream properties");
		};

		std::unique_ptr<IWMOutputMediaProps, decltype(&XAux::COM_deleter)> upMediaProps{ pMediaProps, XAux::COM_deleter };

		pMediaProps->GetMediaType(NULL, &dwPropsBufferSize);

		std::unique_ptr<BYTE[]> upPropsData{ new BYTE[dwPropsBufferSize] };

		if FAILED(pMediaProps->GetMediaType((WM_MEDIA_TYPE*)upPropsData.get(), &dwPropsBufferSize))
		{
			throw XException(L"XWMADecoder()::_OpenFile(): can't get media type");
		};

		pMediaType = (WM_MEDIA_TYPE*)upPropsData.get();

		// ����������, ��� ��� ���� �������� �����:

		if ((pMediaType->majortype != WMMEDIATYPE_Audio) || (pMediaType->formattype != WMFORMAT_WaveFormatEx))
		{
			throw XException(L"XWMADecoder()::_OpenFile(): unsupported file type");
		};

		// ��������� ������ �����:
		WAVEFORMATEX *pWaveFormatEx = (WAVEFORMATEX*)pMediaType->pbFormat;

		m_SoundFormat.wFormatTag = WAVE_FORMAT_PCM;
		m_SoundFormat.nChannels = pWaveFormatEx->nChannels;
		m_SoundFormat.nSamplesPerSec = pWaveFormatEx->nSamplesPerSec;
		m_SoundFormat.wBitsPerSample = pWaveFormatEx->wBitsPerSample;
		m_SoundFormat.nBlockAlign = m_SoundFormat.nChannels * m_SoundFormat.wBitsPerSample / 8;
		m_SoundFormat.nAvgBytesPerSec = m_SoundFormat.nSamplesPerSec * m_SoundFormat.nBlockAlign;
		m_SoundFormat.cbSize = 0;
	};

	// 3. ��������� ����� OPEN_GET_SIZE & OPEN_LOAD_SAMPLES
	
	if ( (Mode & (uint8_t)OpenMode::OPEN_GET_SIZE) || (Mode & (uint8_t)OpenMode::OPEN_LOAD_SAMPLES))
	{
		// ������������� (���������� ����� ����� -- ��� ����� ������� � ����� ������, ����� �������� ��� ���):
		
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
					throw XException(L"XWMADecoder()::_OpenFile(): unpacking error");

					break;
				};

			pBuffer->GetLength(&dwSamplesLength);

			m_nFileSize += dwSamplesLength;

			if ((Mode & (uint8_t)OpenMode::OPEN_LOAD_SAMPLES))
			{
				// ���������� ���� �������� - ��������� ����� � ������:
				listBuffers.emplace_back(pBuffer, XAux::COM_deleter);
			}
			else
			{
				// �������� �� ����� - ����������� ������.
				pBuffer->Release();
			}
		};

		pSyncReader->SetRange(0, 0);

		if (Mode & (uint8_t)OpenMode::OPEN_LOAD_SAMPLES)
		{
			// ��������������� ��������. �������� ���������� �������.

			m_upSamples.reset(new BYTE[m_nFileSize]);

			DWORD dwOffset{ 0 };

			for (const auto &ptr_buf : listBuffers)
			{
				DWORD dwLength;
				BYTE* pData;

				ptr_buf->GetBufferAndLength(&pData, &dwLength);

				std::copy(pData, pData + dwLength, m_upSamples.get() + dwOffset);
				dwOffset += dwLength;
			}
		};
	}

	// 4. ��������� ����� OPEN_STREAM

	if ( (Mode & (uint8_t)OpenMode::OPEN_STREAM) )
	{
		// ���������� � �������������. ��������� upGH (��� ����� ������ upStream), ��������� ����� � �����.

		m_upWMReader = std::move(upSyncReader);
		m_upStream = std::move(upStream);

		upGH.release();
	}

	// ��� OK.
}

void XWMADecoder::_ResetInternalData()
{
	m_strFileName = L"";
	m_nFileSize = 0;
	m_bAssigned = false;
	m_bLoaded = false;
	m_bDecoding = false;
	m_nRefCount = 0;
	m_nBytesRemain = 0;
	m_nRemainOffset = 0;

	m_upSamples.reset();
	m_upBuffer.reset();
	m_upWMReader.reset();
	m_upStream.reset();
}

void XWMADecoder::AssignFile(const wchar_t* pFileName, XSoundDecoder::AssignHint Hint)
{
	std::unique_lock lock{ m_mtxMain };

	if (m_bAssigned)
	{
		// �������� ��������� ������:
		throw XException(L"XWMADecoder()::AssignFile(): file is already assigned");
	}

	// ��������� ��������� ������ ������.
	m_hintMode = Hint;

	try
	{
		// � ����������� �� ���������, ��������� � ������ �������:
		uint8_t nMode{ 0 };

		if (Hint == XSoundDecoder::AssignHint::HINT_STREAM_ONLY)
		{
			// ��� ���������� ��������������� ���������� ������ �������� ������.
			nMode = (uint8_t)OpenMode::OPEN_GENERIC | (uint8_t)OpenMode::OPEN_READ_FORMAT;
		} else
		if ((Hint == XSoundDecoder::AssignHint::HINT_FETCH_ONLY) || (Hint == XSoundDecoder::AssignHint::HINT_MIXED))
		{
			// ��� ��������� ������� - ����� ��� � �����, �� ������ �� ������.
			nMode = (uint8_t)OpenMode::OPEN_GENERIC | (uint8_t)OpenMode::OPEN_READ_FORMAT | (uint8_t)OpenMode::OPEN_GET_SIZE;
		}
		else
		{
			// ���������� ������?
			throw XException(L"XWMADecoder()::AssignFile(): can't resolve hint flag");
		}

		_OpenFile(pFileName, nMode);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XWMADecoder::AssignFile(): can't assign file '%s'", pFileName);
	}

	m_bAssigned = true;
	m_bLoaded = false;
	m_bDecoding = false;
	m_strFileName = pFileName;
}

bool XWMADecoder::ReleaseFile()
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// �� ��� ��������.
		return false;
	}

	// ����������� ������:
	_ResetInternalData();

	return true;
}

bool XWMADecoder::IsAssigned()
{
	std::shared_lock lock{ m_mtxMain };

	return m_bAssigned;
}

bool XWMADecoder::Load()
{
	std::unique_lock lock{ m_mtxMain };

	if ( (!m_bAssigned) || (m_bDecoding) || (m_hintMode == XSoundDecoder::AssignHint::HINT_STREAM_ONLY) )
	{
		// ���� �� ��� ������ (������ ���������), ������������ ��� ��� ���������� �������� �� ��������������.
		return false;
	};

	if (m_bLoaded)
	{
		// ��� ��������.
		return true;
	}

	// �������� ���������:
	try
	{
		_OpenFile(m_strFileName.c_str(), (uint8_t)OpenMode::OPEN_LOAD_SAMPLES);
	}
	catch (const XException& e)
	{
		// �� �����-�� �������� ��������� �� �������, ���� ������ �������� � ���� �������.

		UNREFERENCED_PARAMETER(e);

		return false;
	}

	m_bLoaded = true;

	return true;
}

bool XWMADecoder::Unload()
{
	std::unique_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded) || (m_nRefCount))
	{
		// ������ ������ ���� ��������� � ������� ������ ����� 0.
		return false;
	}

	m_upSamples.reset(nullptr);

	m_bLoaded = false;

	return true;
}

bool XWMADecoder::IsLoaded()
{
	std::shared_lock lock{ m_mtxMain };

	return (m_bLoaded && m_bAssigned);
}

uint32_t XWMADecoder::GetSize()
{
	std::shared_lock lock{ m_mtxMain };

	// ���� ��������� �� �������������� -- ���������� 0.

	if (m_hintMode == XSoundDecoder::AssignHint::HINT_STREAM_ONLY)
	{
		return 0;
	}

	return m_nFileSize;
}

bool XWMADecoder::GetFormat(WAVEFORMATEX & refFormat)
{
	std::shared_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// ���� �� ��� ������, ������ ����������.
		return false;
	}

	refFormat = m_SoundFormat;

	return true;
}

bool XWMADecoder::GetData(std::unique_ptr<BYTE[]> &refData)
{
	std::shared_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded) || (m_hintMode == XSoundDecoder::AssignHint::HINT_STREAM_ONLY))
	{
		// ���� �� ��� ��������, ���� �������� �� �������������� --  ������ ����������.
		return false;
	}

	// ���������� ����� ������.

	refData = std::make_unique<BYTE[]>(m_nFileSize);

	std::copy(m_upSamples.get(), m_upSamples.get() + m_nFileSize, refData.get());

	return true;
}

bool XWMADecoder::GetDataDirect(BYTE*& refData)
{
	std::unique_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded) || (m_hintMode == XSoundDecoder::AssignHint::HINT_STREAM_ONLY))
	{
		// ������ ����������.
		return false;
	}

	// ���������� ������ ��������� � ����������� �������.

	refData = m_upSamples.get();
	m_nRefCount++;

	return true;
}

void XWMADecoder::FreeData()
{
	std::unique_lock lock{ m_mtxMain };

	if (m_nRefCount <= 0)
	{
		throw XException(L"XWMADecoder::FreeData(): internal counter is zero");
	}

	m_nRefCount--;
}

void XWMADecoder::DecodeStart()
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// ���� �� ��� ������:
		throw XException(L"XWMADecoder()::DecodeStart(): file is not opened");
	}

	if (m_bDecoding)
	{
		// �������� ������������ ������:
		throw XException(L"XWMADecoder()::DecodeStart(): file '%s' is already decoding", m_strFileName);
	}

	if (m_hintMode == XSoundDecoder::AssignHint::HINT_FETCH_ONLY)
	{
		// ����� -- ������ ��������.
		throw XException(L"XWMADecoder()::DecodeStart(): file '%s' is for fetch-only mode", m_strFileName);
	}

	try
	{
		// �������� ������� ��� ����������:
		_OpenFile(m_strFileName.c_str(), (uint8_t)OpenMode::OPEN_STREAM);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XWMADecoder::DecodeStart(): can't start decoding");
	}

	m_bDecoding = true;
}

uint32_t XWMADecoder::DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount)
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bDecoding)
	{
		return 0;
	}

	uint32_t nBytesUnpacked{ 0 };

	// ���� � �������� ������ �������� ������ � ������ - ��������� �� � ���� ���.
	if (m_upBuffer != nullptr)
	{
		uint8_t *ptr;

		m_upBuffer->GetBuffer(&ptr);

		std::copy(ptr + m_nRemainOffset, ptr + m_nRemainOffset + m_nBytesRemain, refDestBuffer.get());
		nBytesUnpacked += m_nBytesRemain;

		m_upBuffer = nullptr;
	}

	// �������������, ���� �� �������� ����� ��� �� �������� ������.
	while (nBytesUnpacked < nCount)
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

		hResult = m_upWMReader->GetNextSample(0, &pBuffer, &qwSampleTime, &qwSampleDuration, &dwFlags, &dwOutputNum, &wStreamNum);

		if (hResult == NS_E_NO_MORE_SAMPLES)
		{
			break;
		}
		else
			if (hResult != S_OK)
			{
				throw XException(L"XWMADecoder::DecodeBytes(): unpacking error");

				break;
			};

		m_upBuffer.reset(pBuffer);

		// ��������� ��������� ������ ������...

		m_upBuffer->GetLength(&dwSamplesLength);
		m_upBuffer->GetBuffer(&pSource);

		// We can't copy more bytes than can be stored in the buffer.
		uint32_t nBytesToCopy{ min(dwSamplesLength, nCount - nBytesUnpacked) };

		std::copy(pSource, pSource + nBytesToCopy, refDestBuffer.get() + nBytesUnpacked);

		nBytesUnpacked += nBytesToCopy;

		m_nBytesRemain = dwSamplesLength - nBytesToCopy;
		m_nRemainOffset = nBytesToCopy;

		if (m_nBytesRemain == 0)
		{
			// ����� ��������� �����������, �����������.
			m_upBuffer = nullptr;
		};

	}

	return nBytesUnpacked;
}

bool XWMADecoder::DecodeStop()
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bDecoding)
	{
		return false;
	}

	if (m_upWMReader != nullptr)
	{
		// ��������� �����.
		m_upWMReader->Close();
	};

	// ����������� ������.
	m_upBuffer = nullptr;
	m_upWMReader = nullptr;
	m_upStream = nullptr;

	m_bDecoding = false;

	return true;
}

bool XWMADecoder::IsDecoding()
{
	std::shared_lock lock{ m_mtxMain };

	return m_bDecoding;
}
