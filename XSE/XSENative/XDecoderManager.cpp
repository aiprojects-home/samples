#include "pch.h"
#include "XDecoderManager.h"

#include "XPCMDecoder.h"
#include "XWMADecoder.h"
#include "XException.h"

std::unique_ptr<XDecoderManager> XDecoderManager::m_upCurrent{ nullptr };
std::mutex                       XDecoderManager::m_mtxStaticLock;

XDecoderManager::XDecoderManager()
{
	m_vecDecoders = {
						std::function<CreateDecoderFunc>(XPCMDecoder::CreateStatic),
						std::function<CreateDecoderFunc>(XWMADecoder::CreateStatic)
	};

}

XDecoderManager::~XDecoderManager()
{

}

XDecoderManager& XDecoderManager::Current()
{
	// ����������� ����� - ����������� ����������.
	std::unique_lock lock{ m_mtxStaticLock };

	if (!m_upCurrent.get())
	{
		// ������ �����.
		m_upCurrent = std::unique_ptr<XDecoderManager>(new XDecoderManager());
	}

	return *m_upCurrent.get();

}

XSoundDecoder* XDecoderManager::OpenFile(const wchar_t *pFileName) const
{
	std::shared_lock lock{ m_mtxMainLock };

	std::vector<XException> vecErrors;

	for (size_t i = 0; i < m_vecDecoders.size(); ++i)
	{
		// �� ���� ���������.

		XSoundDecoder *pDecoder = m_vecDecoders[i]();

		try
		{
			pDecoder->AssignFile(pFileName);
		}
		catch (XException &e)
		{
			vecErrors.push_back(e);
		}

		if (pDecoder->IsAssigned())
		{
			// ������, ������� �������.
			return pDecoder;
		};

		// ������� �� �������� - ������� ������ �� ������.
		delete pDecoder;
	}

	std::wstring strErrorSummary;

	// �� ���� �� �������� - ������� ����������� ����������.
	for (size_t i = 0; i < vecErrors.size(); ++i)
	{
		strErrorSummary += vecErrors[i].GetError();
		strErrorSummary += L"\n";
	}

	strErrorSummary = std::wstring(L"XDecoderManager::OpenFile(): unsupported file type\nSummary: \n") + strErrorSummary;

	throw XException(strErrorSummary.c_str());
}
