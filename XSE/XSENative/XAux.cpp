#include "pch.h"
#include "XAux.h"

void XAux::HGLOBAL_deleter(HGLOBAL* hMem)
{
	if (hMem)
		GlobalFree(reinterpret_cast<HGLOBAL>(hMem));

};

void XAux::COM_deleter(IUnknown* pInterface)
{
	if (pInterface)
		pInterface->Release();

};