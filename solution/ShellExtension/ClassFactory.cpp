#include "ClassFactory.h"
#include "ContextMenuExt.h"
#include <new>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

extern long g_cDllRef;

ClassFactory::ClassFactory(void) : m_cRef(1)
{
	InterlockedIncrement(&g_cDllRef);
}

ClassFactory::~ClassFactory(void)
{
	InterlockedDecrement(&g_cDllRef);
}

//IUnknow

IFACEMETHODIMP ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
	static const QITAB qit[] = 
	{
		QITABENT(ClassFactory, IClassFactory),
		{ 0 },
	};

	return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ClassFactory::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) ClassFactory::Release()
{
	ULONG ref =  InterlockedDecrement(&m_cRef);

	if(ref == 0)
	{
		delete this;
	}

	return ref;
}

//IClassFactory

IFACEMETHODIMP ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	HRESULT hr = CLASS_E_NOAGGREGATION;

	if(pUnkOuter == NULL)
	{
		hr = E_OUTOFMEMORY;

		ContextMenuExt *pExt = new (std::nothrow) ContextMenuExt();
		if(pExt)
		{
			hr = pExt->QueryInterface(riid, ppv);
			pExt->Release();
		}
	}

	return hr;
}

IFACEMETHODIMP ClassFactory::LockServer(BOOL fLock)
{
	if(fLock)
	{
		InterlockedIncrement(&g_cDllRef);
	}
	else
	{
		InterlockedDecrement(&g_cDllRef);
	}

	return S_OK;
}
