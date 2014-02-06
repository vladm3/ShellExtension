#pragma once
#include <Windows.h>
#include <mutex>
#include "resource.h"
#include <CommCtrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


class ProgressDialog
{
public:
	ProgressDialog(void);
	ProgressDialog(HINSTANCE hInst, HWND hWndParent);
	static void openDialog(HINSTANCE hInst, HWND hWndParent);
	~ProgressDialog(void);

	//HWND getProgressBarHandle();
	//unsigned int getProgressBarMax();
	//unsigned int getProgressBarValue();

	//void setProgressBarValue(unsigned int val);
	//void setProgressBarMax(unsigned int max);

public:
	std::mutex syncObj;

	static HWND parent;
	static HINSTANCE instance;
	static HWND pBar;

	static LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
};

void ProgressDialog::openDialog(HINSTANCE hInst, HWND hWndParent)
{
	ProgressDialog::instance = hInst;
	ProgressDialog::parent = hWndParent;

	DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), hWndParent, reinterpret_cast<DLGPROC>(ProgressDialog::DlgProc));
}

LRESULT CALLBACK ProgressDialog::DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	RECT rcClient;

	GetClientRect(hWndDlg, &rcClient);

	INITCOMMONCONTROLSEX InitCtrlEx;

	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&InitCtrlEx);

	switch(Msg)
	{
	case WM_INITDIALOG:
		ProgressDialog::pBar = CreateWindowEx(0, PROGRESS_CLASS, NULL, 
			WS_CHILD | WS_VISIBLE, 20, (rcClient.bottom - rcClient.top) / 2 - 25, 
			(rcClient.right - rcClient.left) - 40, 20, 
			hWndDlg, NULL, ProgressDialog::instance, NULL);
		return TRUE;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			return TRUE;
		default:
			break;
		}
		break;

	default:
		break;
	}

	return FALSE;
}