#pragma once

#include "XException.h"

// Интерфейс для внешних уведомлений.
class IXAudioPlayerCallback
{
public:

	// Вызывается когда завершается потоковое воспроизведение (не вручную, а при достижении конца).
	virtual void OnStreamingEnd(uint16_t id) = 0;

	// Вызов при внутренней ошибке.
	virtual void OnError(const uint16_t id, const XException e) = 0;
};
