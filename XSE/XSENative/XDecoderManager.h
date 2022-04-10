#pragma once

#include "XGlobals.h"
#include "XSoundDecoder.h"

class XDecoderManager
{
private:
	static std::unique_ptr<XDecoderManager> m_upCurrent;
	
	static  std::mutex m_mtxStaticLock;
	mutable std::shared_mutex m_mtxMainLock;

	std::vector<std::function<CreateDecoderFunc>> m_vecDecoders;

private:

	XDecoderManager();

public:

	~XDecoderManager();

	static XDecoderManager& Current();

	XSoundDecoder* OpenFile(const wchar_t *pFileName, XSoundDecoder::AssignHint Hint) const;
};

