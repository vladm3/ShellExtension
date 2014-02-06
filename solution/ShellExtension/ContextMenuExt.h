#pragma once

#include <Windows.h>
#include <ShlObj.h>

class ContextMenuExt : public IShellExtInit, public IContextMenu
{
public:
	//IUnknown methods
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	//IShellExtInit method
	IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgId);

	//IContextMenu methods
	IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
	IFACEMETHODIMP GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

	ContextMenuExt(void);
	~ContextMenuExt(void);

private:
	long m_cRef;

	size_t selectedFilesCount;
	wchar_t **selectedFiles;

	wchar_t *menuText;
	char *verb;
	wchar_t *wverb;
	char *verbCanonicalName;
	wchar_t *wverbCanonicalName;
	char *verbHelpText;
	wchar_t *wverbHelpText;

	void QueryLogging(HWND hWnd);
};

