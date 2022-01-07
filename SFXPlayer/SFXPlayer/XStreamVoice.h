#pragma once

#include "framework.h"
#include "XVoiceBase.h"
#include <atomic>
#include <mutex>

class XStreamVoice : public XVoiceBase
{
private:
	std::unique_ptr<BYTE[]> m_spBuffer[3];
	uint32_t                m_nBufferSize;
	uint8_t                 m_nBufferToWrite;
	std::atomic<uint8_t>    m_nBuffersLeft;

	std::mutex              m_DecoderLock;
	std::condition_variable m_cvDecoder;

	std::unique_ptr<std::thread> m_spDecoderThread;

	bool m_bEOS;        // no more samples
	bool m_bUnloadFlag; // unload thread
	bool m_bSubmitFlag; // time to submit another buffer
	bool m_bPlaying;    // true, if thread are working

public:

	XStreamVoice() = delete;
	XStreamVoice(XAudioPlayer& audioPlayer, XVoicePool& voicePool, XSoundBank& soundBank, WAVEFORMATEX& wfex, uint16_t id);
	~XStreamVoice();

	void CreateAndPlay();
	void StopAndUnload();

	virtual void OnBufferEnd(void * pBufferContext);

private:
	friend bool PredStreamDecoder(XStreamVoice& pObject);
	void SubmitBuffer();
	void ThreadStreamDecoder();
};

