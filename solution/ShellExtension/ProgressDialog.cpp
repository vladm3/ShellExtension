#include "ProgressDialog.h"

ProgressDialog::ProgressDialog(void)
{
}

ProgressDialog::ProgressDialog(HINSTANCE hInst, HWND hWndParent)
{
	this->openDialog(hInst, hWndParent);
}

ProgressDialog::~ProgressDialog(void)
{
}