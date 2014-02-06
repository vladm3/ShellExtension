#include "ContextMenuExt.h"
#include <strsafe.h>
#include <Shlwapi.h>
#include "FileInfoLogger.h"
#include "resource.h"
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

extern HINSTANCE g_hInst;
extern long g_cDllRef;
extern HWND hDlgWnd;

LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG:
		hDlgWnd = hWndDlg;
		SendMessage(GetDlgItem(hWndDlg, IDC_PROGRESS_BAR), PBM_SETRANGE32, 0, 1);
		SendMessage(GetDlgItem(hWndDlg, IDC_PROGRESS_BAR), PBM_SETSTEP, 1, 0);
		return TRUE;

	case WM_NOTIFY:
		switch(((NMHDR*)lParam)->code)
		{
		case NM_CLICK:
		{
			NMLINK* pNMLink = (NMLINK*)lParam;
			LITEM lItem = pNMLink->item;

			if(wParam == IDC_SYSLINK_LOGFILE)
			{
				ShellExecute(0, 0, lItem.szUrl, 0, 0, SW_SHOW);
				EndDialog(hWndDlg, 0);
			}
			return TRUE;
		}
		default:
			break;
		}
		break;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			return TRUE;
		case 9:
			EnableWindow(GetDlgItem(hWndDlg, IDOK), TRUE);
			SendMessage(GetDlgItem(hWndDlg, IDC_PROGRESS_BAR), PBM_SETPOS, lParam, 0);
			return TRUE;
		case 10:
			SendMessage(GetDlgItem(hWndDlg, IDC_PROGRESS_BAR), PBM_SETRANGE32, 0, lParam);
			SendMessage(GetDlgItem(hWndDlg, IDC_PROGRESS_BAR), PBM_STEPIT, 0, 0);
			return TRUE;
		case 11:
			PLITEM res;
			SendMessage(GetDlgItem(hWndDlg, IDC_SYSLINK_LOGFILE), LM_GETITEM, NULL, (LPARAM)res);
			MessageBox(0, res->szUrl, res->szUrl, 0);
			return TRUE;
		default:
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hWndDlg, 0);
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

ContextMenuExt::ContextMenuExt(void) : 
	m_cRef(1),
    menuText(L"&Calculate the Sum"),
    verb("chksumcalc"),
    wverb(L"chksumcalc"),
    verbCanonicalName("CalcCheckSum"),
    wverbCanonicalName(L"CalcCheckSum"),
    verbHelpText("Calculate the Sum"),
    wverbHelpText(L"Calculate the Sum"),
	selectedFiles(nullptr),
	selectedFilesCount(0)
{
	InterlockedIncrement(&g_cDllRef);
}

ContextMenuExt::~ContextMenuExt(void)
{
	if(selectedFiles)
	{
		for(size_t i = 0; i < selectedFilesCount; i++)
		{
			delete [] selectedFiles[i];
		}

		delete [] selectedFiles;
		selectedFiles = nullptr;

		selectedFiles = 0;
	}

	InterlockedDecrement(&g_cDllRef);
}

void ContextMenuExt::QueryLogging(HWND hWnd)
{
	std::thread thr1(
		[](HWND hWND)
		{
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG), hWND, reinterpret_cast<DLGPROC>(DlgProc));
		}, hWnd);
	thr1.detach();

	std::vector<std::wstring> vec;
	vec.reserve(selectedFilesCount);
	for(size_t i = 0; i < selectedFilesCount; ++i)
	{
		vec.push_back(std::wstring(selectedFiles[i]));
	}

	std::thread thr(
		[](std::vector<std::wstring> fn)
		{
			FileInfoLogger logger(fn, L"C:/chksumlog.txt");
			logger.logInfo();
		}, vec);
	thr.detach();
}

//IUnknown

IFACEMETHODIMP ContextMenuExt::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(ContextMenuExt, IContextMenu),
        QITABENT(ContextMenuExt, IShellExtInit), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) ContextMenuExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) ContextMenuExt::Release()
{
	ULONG ref = InterlockedDecrement(&m_cRef);

	if(ref == 0)
	{
		delete this;
	}

	return ref;
}

//IShellExtInit

IFACEMETHODIMP ContextMenuExt::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
    if (NULL == pDataObj)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stm;

    if (SUCCEEDED(pDataObj->GetData(&fe, &stm)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop != NULL)
        {
            UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            if (nFiles > 0)
            {
				if(selectedFiles)
				{
					for(size_t i = 0; i < selectedFilesCount; i++)
					{
						delete [] selectedFiles[i];
					}

					delete [] selectedFiles;
					selectedFiles = nullptr;

					selectedFilesCount = 0;
				}

				selectedFilesCount = nFiles;

				selectedFiles = new wchar_t*[selectedFilesCount];
				for(size_t i = 0; i < selectedFilesCount; i++)
				{
					selectedFiles[i] = new wchar_t[MAX_PATH];
				}


				for(size_t i = 0; i < selectedFilesCount; i++)
				{
					if(0 == DragQueryFile(hDrop, (UINT)i, selectedFiles[i], MAX_PATH))
					{
						hr = E_FAIL;
						break;
					}

					hr = S_OK;
				}
            }

            GlobalUnlock(stm.hGlobal);
        }

        ReleaseStgMedium(&stm);
    }

    return hr;
}

//IContextMenu

IFACEMETHODIMP ContextMenuExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (CMF_DEFAULTONLY & uFlags)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
    }

    MENUITEMINFO mii = { sizeof(mii) };
    mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE;
    mii.wID = idCmdFirst + IDM_DISPLAY;
    mii.fType = MFT_STRING;
	mii.dwTypeData = menuText;
    mii.fState = MFS_ENABLED;

    if (!InsertMenuItem(hMenu, indexMenu, TRUE, &mii))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    MENUITEMINFO sep = { sizeof(sep) };
    sep.fMask = MIIM_TYPE;
    sep.fType = MFT_SEPARATOR;

    if (!InsertMenuItem(hMenu, indexMenu + 1, TRUE, &sep))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_DISPLAY + 1));
}

IFACEMETHODIMP ContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    BOOL fUnicode = FALSE;

    if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
    {
        if (pici->fMask & CMIC_MASK_UNICODE)
        {
            fUnicode = TRUE;
        }
    }

    if (!fUnicode && HIWORD(pici->lpVerb))
    {
		if (StrCmpIA(pici->lpVerb, verb) == 0)
        {
			QueryLogging(pici->hwnd);
        }
        else
        {
            return E_FAIL;
        }
    }
    else if (fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW))
    {
		if (StrCmpIW(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW, wverb) == 0)
        {
            QueryLogging(pici->hwnd);
        }
        else
        {
            return E_FAIL;
        }
    }
    else
    {
        if (LOWORD(pici->lpVerb) == IDM_DISPLAY)
        {
			QueryLogging(pici->hwnd);
        }
        else
        {
            return E_FAIL;
        }
    }

    return S_OK;
}

IFACEMETHODIMP ContextMenuExt::GetCommandString(UINT_PTR idCommand, UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_INVALIDARG;

    if (idCommand == IDM_DISPLAY)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, wverbHelpText);
            break;
        case GCS_VERBW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, wverbCanonicalName);
            break;
        default:
            hr = S_OK;
		}
	}

	return hr;
}