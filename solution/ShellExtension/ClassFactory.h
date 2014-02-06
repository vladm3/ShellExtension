#pragma once

#include <Unknwn.h>
#include <Windows.h>

class ClassFactory : public IClassFactory
{
public:
	//IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	//IClassFactory methods
	IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
	IFACEMETHODIMP LockServer(BOOL lock);

	ClassFactory(void);

protected:
	~ClassFactory(void);

private:
	long m_cRef;
};

