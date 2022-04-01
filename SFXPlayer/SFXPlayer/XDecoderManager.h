#pragma once

#include "framework.h"
#include <memory>
#include <shared_mutex>
#include <functional>
#include "XSoundDecoder.h"

class XDecoderManager
{
private:
	static std::unique_ptr<XDecoderManager> m_upCurrent;
	static std::mutex m_mtxStaticLock;

	std::mutex m_mtxMainLock;

	std::vector<std::function<CreateDecoderFunc>> m_vecDecoders;

private:

	XDecoderManager();

public:

	~XDecoderManager();

	static XDecoderManager& Current();

	XSoundDecoder* OpenFile(const wchar_t *pFileName);
};

