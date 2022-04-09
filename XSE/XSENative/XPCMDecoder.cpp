#include "pch.h"
#include "XPCMDecoder.h"
#include "XAux.h"
#include "XException.h"
#include "XEFMReader.h"

XPCMDecoder::XPCMDecoder()
{
	_ResetInternalData();
}

XPCMDecoder::~XPCMDecoder()
{

}

XSoundDecoder* XPCMDecoder::CreateStatic()
{
	return new XPCMDecoder();
}

void XPCMDecoder::_OpenFile(const wchar_t* pFileName, bool bLoad)
{
	XEFMReader reader;

	// Пытаемся открыть файл:
	try
	{
		reader.Open(pFileName);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XPCMDecoder()::_OpenFile(): can't open file '%s'", pFileName);

	}

	// Идентификаторы чанков WAVE :
	const DWORD WAV_RIFF = 0x46464952;
	const DWORD WAV_WAVE = 0x45564157;
	const DWORD WAV_fmt  = 0x20746D66;
	const DWORD WAV_data = 0x61746164;

	// Заголовок WAVE-чанка:
	struct WAV_CHUNK
	{
		DWORD chkId;   // chunk id
		DWORD chkSize; // chunk size
	};

	WAV_CHUNK WaveChk;
	DWORD     dwSign, dwRiffSize;
	DWORD     dwFormatOffset = 0, dwDataOffset = 0, dwFormatLength = 0, dwDataLength = 0;

	// Читаем первый чанк - RIFF:

	reader.ReadBytes(reinterpret_cast<char*>(&WaveChk), sizeof(WAV_CHUNK));

	if (WaveChk.chkId != WAV_RIFF)
	{
		throw XException(L"XPCMDecoder()::_OpenFile(): unsupported file type");
	};

	dwRiffSize = WaveChk.chkSize + sizeof(WAV_CHUNK);

	// Читаем сигнатуру WAVE-формы:

	reader.ReadBytes(reinterpret_cast<char*>(&dwSign), sizeof(DWORD));

	if (dwSign != WAV_WAVE)
	{
		throw XException(L"XPCMDecoder()::_OpenFile(): unsupported file type");
	};

	// Скачем по всем чанкам до конца файла:

	do
	{
		reader.ReadBytes(reinterpret_cast<char*>(&WaveChk), sizeof(WAV_CHUNK));

		switch (WaveChk.chkId)
		{
		case WAV_fmt:
		{
			// чанк с данными (описание формата):
			dwFormatOffset = reader.Tell();
			dwFormatLength = WaveChk.chkSize;

			break;
		};
		case WAV_data:
		{
			// чанк с семплами:
			dwDataOffset = reader.Tell();
			dwDataLength = WaveChk.chkSize;

			break;
		};

		};

		// "Перепрыгиваем" текущий чанк...
	} while (reader.Seek(WaveChk.chkSize, XEFMReader::XSEEK_TYPE::XSEEK_CURRENT));

	if (!dwFormatOffset || !dwDataOffset)
	{
		// Нужные чанки не были найдены:
		throw XException(L"XPCMDecoder()::_OpenFile(): format type chunk and/or wave data not detected");
	};

	// Читаем формат файла в структуру WAVEFORMATEX:
	std::unique_ptr<BYTE[]> upFormat(new BYTE[dwFormatLength]);

	LPWAVEFORMATEX lpFormat = reinterpret_cast<LPWAVEFORMATEX>(upFormat.get());

	reader.Seek(dwFormatOffset);
	reader.ReadBytes(reinterpret_cast<char*>(lpFormat), dwFormatLength);

	// Проверяем формат данных (должны быть незажатые) & сохраняем:
	if (lpFormat->wFormatTag != WAVE_FORMAT_PCM)
	{
		// Формат - не простой незажатый PCM:
		throw XException(L"XPCMDecoder()::_OpenFile(): file must be uncompressed");
	}

	m_SoundFormat.wFormatTag = WAVE_FORMAT_PCM;
	m_SoundFormat.nChannels = lpFormat->nChannels;
	m_SoundFormat.nSamplesPerSec = lpFormat->nSamplesPerSec;
	m_SoundFormat.wBitsPerSample = lpFormat->wBitsPerSample;
	m_SoundFormat.nBlockAlign = m_SoundFormat.nChannels * m_SoundFormat.wBitsPerSample / 8;
	m_SoundFormat.nAvgBytesPerSec = m_SoundFormat.nSamplesPerSec * m_SoundFormat.nBlockAlign;
	m_SoundFormat.cbSize = 0;

	m_nFileSize = dwDataLength;
	m_nDataOffset = dwDataOffset;

	if (!bLoad)
	{
		// Загружать не надо, формат и длину получили. Выходим.
		return;
	};

	// Читаем семплы:
	m_upSamples.reset(new BYTE[dwDataLength]);

	bool bOK = reader.Seek(dwDataOffset);
	uint32_t bytes_read = reader.ReadBytes(reinterpret_cast<char*>(m_upSamples.get()), dwDataLength);
}

void XPCMDecoder::_ResetInternalData()
{
	m_strFileName = L"";
	m_nFileSize = 0;
	m_bAssigned = false;
	m_bLoaded = false;
	m_bDecoding = false;
	m_nRefCount = 0;
	m_nBytesRemain = 0;
	m_nDataOffset = 0;
	m_nCurrentOffset = 0;

	m_upSamples.reset();
}

void XPCMDecoder::AssignFile(const wchar_t* pFileName)
{
	std::unique_lock lock{ m_mtxMain };

	if (m_bAssigned)
	{
		// Повторно открывать нельзя:
		throw XException(L"XPCMDecoder()::AssignFile(): file is already assigned");
	}

	try
	{
		// Пытаемся открыть файл без загрузки:

		_OpenFile(pFileName, false);
	}
	catch (const XException& e)
	{
		throw XException(e, L"XPCMDecoder::AssignFile(): can't assign file '%s'", pFileName);
	}

	m_bAssigned = true;
	m_bLoaded = false;
	m_bDecoding = false;
	m_strFileName = pFileName;
}

bool XPCMDecoder::ReleaseFile()
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// Не был назначен.
		return false;
	}

	// Освобождаем память:
	_ResetInternalData();

	return true;
}

bool XPCMDecoder::IsAssigned()
{
	std::shared_lock lock{ m_mtxMain };

	return m_bAssigned;
}

bool XPCMDecoder::Load()
{
	std::unique_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (m_bDecoding))
	{
		// Файл не был открыт (нечего загружать) или декодируется.
		return false;
	};

	if (m_bLoaded)
	{
		// Уже загружен.
		return true;
	}

	// Пытаемся загрузить:
	try
	{
		_OpenFile(m_strFileName.c_str(), true);
	}
	catch (const XException& e)
	{
		// По каким-то причинам загрузить не удалось, хотя назначение было удачным.

		UNREFERENCED_PARAMETER(e);

		return false;
	}

	m_bLoaded = true;

	return true;
}

bool XPCMDecoder::Unload()
{
	std::unique_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded) || (m_nRefCount))
	{
		// Данные должны быть загружены и счетчик ссылок равен 0.
		return false;
	}

	m_upSamples.reset(nullptr);

	m_bLoaded = false;

	return true;
}

bool XPCMDecoder::IsLoaded()
{
	std::shared_lock lock{ m_mtxMain };

	return (m_bLoaded && m_bAssigned);
}

uint32_t XPCMDecoder::GetSize()
{
	std::shared_lock lock{ m_mtxMain };

	return m_nFileSize;
}

bool XPCMDecoder::GetFormat(WAVEFORMATEX & refFormat)
{
	std::shared_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// Файл не был открыт, нечего возвращать.
		return false;
	}

	refFormat = m_SoundFormat;

	return true;
}

bool XPCMDecoder::GetData(std::unique_ptr<BYTE[]> &refData)
{
	std::shared_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded))
	{
		// Файл не был загружен, нечего возвращать.
		return false;
	}

	// Возвращаем копию данных.

	refData = std::make_unique<BYTE[]>(m_nFileSize);

	std::copy(m_upSamples.get(), m_upSamples.get() + m_nFileSize, refData.get());

	return true;
}

bool XPCMDecoder::GetDataDirect(BYTE*& refData)
{
	std::unique_lock lock{ m_mtxMain };

	if ((!m_bAssigned) || (!m_bLoaded))
	{
		// Нечего возвращать.
		return false;
	}

	// Возвращаем прямой указатель и накручиваем счетчик.

	refData = m_upSamples.get();
	m_nRefCount++;

	return true;
}

void XPCMDecoder::FreeData()
{
	std::unique_lock lock{ m_mtxMain };

	if (m_nRefCount <= 0)
	{
		throw XException(L"XPCMDecoder::FreeData(): internal counter is zero");
	}

	m_nRefCount--;
}

void XPCMDecoder::DecodeStart(WAVEFORMATEX &wfex)
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bAssigned)
	{
		// Файл не был назначен:
		throw XException(L"XPCMDecoder()::DecodeStart(): file is not opened");
	}

	if (m_bDecoding)
	{
		// Повторно декодировать нельзя:
		throw XException(L"XPCMDecoder()::DecodeStart(): file '%s' is already decoding", m_strFileName);
	}

	m_nBytesRemain = m_nFileSize;
	m_nCurrentOffset = 0;
	wfex = m_SoundFormat;

	m_bDecoding = true;
}

uint32_t XPCMDecoder::DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount)
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bDecoding)
	{
		return 0;
	}

	uint32_t nBytesRead = min(nCount, m_nBytesRemain);

	XEFMReader reader;

	// Пытаемся открыть файл и прочитать требуемое количество байт прямо в буфер:
	try
	{
		reader.Open(m_strFileName.c_str());
		reader.Seek(m_nCurrentOffset);
		nBytesRead = reader.ReadBytes(reinterpret_cast<char*>(refDestBuffer.get()), nBytesRead);
		reader.Close();

		m_nCurrentOffset += nBytesRead;
		m_nBytesRemain -= nBytesRead;
	}
	catch (const XException& e)
	{
    	throw XException(e, L"XPCMDecoder()::DecodeBytes(): can't read samples from file '%s'", m_strFileName.c_str());
	}

	return nBytesRead;
}

bool XPCMDecoder::DecodeStop()
{
	std::unique_lock lock{ m_mtxMain };

	if (!m_bDecoding)
	{
		return false;
	}

	m_bDecoding = false;

	return true;
}

bool XPCMDecoder::IsDecoding()
{
	std::shared_lock lock{ m_mtxMain };

	return m_bDecoding;
}
