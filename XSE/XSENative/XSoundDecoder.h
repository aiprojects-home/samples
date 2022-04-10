#pragma once

#include "XGlobals.h"

class XSoundDecoder
{
public:

	// Подсказка для оптимизации настройки декодера.
	enum class AssignHint
	{
		HINT_FETCH_ONLY = 1,  // только для загрузки
		HINT_STREAM_ONLY = 2, // только для потокового воспроизведения
		HINT_MIXED = 3        // оба режима
	};

	virtual ~XSoundDecoder();

	// Метод назначает указанный файл.
	virtual void AssignFile(const wchar_t* pFileName, XSoundDecoder::AssignHint Hint) = 0;
    
	// Метод сбрасывает текущий назначенный файл и освобождает ресурсы.
	virtual bool ReleaseFile() = 0;

	// Метод возвращает TRUE, если файл открыт.
	virtual bool IsAssigned() = 0;

	// Метод загружает данные из файла целиком в память. Если необходимо, происходит распаковка.
	virtual bool Load() = 0;
	
	// Метод выгружает данные из памяти, освобождая ее, если счетчик обращений к памяти равен 0.
	virtual bool Unload() = 0;

	// Метод возвращает TRUE если файл загружен.
	virtual bool IsLoaded() = 0;

	// Метод возвращает размер загруженных данных.
	virtual uint32_t GetSize() = 0;

	// Метод возвращает формат открытого файла.
	virtual bool GetFormat(WAVEFORMATEX & refFormat) = 0;

	// Метод возвращает копию данных.
	virtual bool GetData(std::unique_ptr<BYTE[]> &refData) = 0;
	
	// Метод возвращает прямой указатель на загруженные данные и накручивает счетчик ссылок.
	virtual bool GetDataDirect(BYTE*& refData) = 0;
	
	// Метод скручивает внутренний счетчик обращений к данным.
	virtual void FreeData() = 0;

	// Метод подготавливает открытый файл к декодированию.
	virtual void DecodeStart() = 0;
	
	// Метод распаковывает очередную порцию данных.
	virtual uint32_t DecodeBytes(std::unique_ptr<uint8_t[]>& refDestBuffer, const uint32_t nCount) = 0;

	// Метод останавливает декодирование и освобождает ресурсы.
	virtual bool DecodeStop() = 0;

	// Метод возвращает TRUE, если начат процесс декодирования.
	virtual bool IsDecoding() = 0;

};

typedef XSoundDecoder* CreateDecoderFunc();

