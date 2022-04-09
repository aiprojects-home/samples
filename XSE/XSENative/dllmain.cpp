#include "pch.h"
#include "XSEObject.h"

_declspec(dllexport) int XSECreateStorage(const wchar_t* pDir, const wchar_t* pStorage)
{
	return XSEObject::Current().CreateStorage(pDir, pStorage);
}

_declspec(dllexport) wchar_t* XSEGetLastError()
{
	return const_cast<wchar_t*>(XSEObject::Current().GetLastError());
}

_declspec(dllexport) int XSEGetLastErrorInfo(wchar_t *pBuffer, int nSize)
{
	return (XSEObject::Current().GetLastErrorInfo(pBuffer, nSize));
}

_declspec(dllexport) int XSEInit(const XSE_INIT params)
{
	return XSEObject::Current().Init(params);
}

_declspec(dllexport) int XSEDone()
{
	return XSEObject::Current().Done();
}

_declspec(dllexport) int XSESuspend()
{
	return XSEObject::Current().Suspend();
}

_declspec(dllexport) int XSEResume()
{
	return XSEObject::Current().Resume();
}

_declspec(dllexport) int XSEPlaySimple(uint16_t nId, float fVolume = 1.0f, float fPan = 0.0f)
{
	return XSEObject::Current().PlaySimple(nId, fVolume, fPan);
}

_declspec(dllexport) int XSEPlayStream(uint16_t nId, float fVolume = 1.0f)
{
	return XSEObject::Current().PlayStream(nId, fVolume);
}

_declspec(dllexport) int XSEStopStream(uint16_t nId)
{
	return XSEObject::Current().StopStream(nId);
}

_declspec(dllexport) int XSEStopAll()
{
	return XSEObject::Current().StopAll();
}

_declspec(dllexport) int XSECOMInitializer(bool bInit)
{
	if (bInit)
	{
		// Инициализцаия COM.
		if (FAILED(::CoInitializeEx(NULL, COINIT::COINIT_MULTITHREADED)))
		{
			return -1;
		}
		else
		{
			return 0;
		}

	}
	else
	{
		// Деинициализация.
		::CoUninitialize();
		return 0;
	}
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

