#pragma once

#include "XException.h"

// External callback.
class IXAudioPlayerCallback
{
public:

	// Called when streaming voice finishes playing (not when manually stopped, but reaches the end of data).
	virtual void OnStreamingEnd(uint16_t id) = 0;

	// Called when error occured.
	virtual void OnError(const uint16_t id, const XException e) = 0;
};
