#pragma once

#include "XGlobals.h"

class XAudioCore
{
private:
	static std::unique_ptr<XAudioCore> m_upCurrent;
	
	IXAudio2               *m_pXAudio2;
	IXAudio2MasteringVoice *m_pXAudio2MV;

	std::shared_mutex m_MainLock;
	static std::mutex m_StaticLock;

	XAudioCore();

public:
	~XAudioCore();

	// Возвращает единственную копию объекта.
	static XAudioCore& Current();

	// Инициализация XAudio2.
	void Init();

	// Удаляет объекты XAudio2 & master voice.
	void Done();

	operator IXAudio2*();

	operator IXAudio2MasteringVoice*();
};

