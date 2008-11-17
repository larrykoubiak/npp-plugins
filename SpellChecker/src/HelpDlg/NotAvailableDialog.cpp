/*
This file is part of Explorer Plugin for Notepad++
Copyright (C)2006 Jens Lorenz <jens.plugin.npp@gmx.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "NotAvailableDialog.h"
#include "NativeLang_def.h"


UINT NotAvailableDialog::doDialog(tSCProp* pSCProp)
{
	_pSCProp = pSCProp;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_NOTAVAILABLE_DLG), _hParent, (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK NotAvailableDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
        case WM_INITDIALOG :
		{
			::SetDlgItemText(_hSelf, IDC_EDIT_RELPATH, _pSCProp->szRelPath);

            _urlAspellWin32.init(_hInst, _hSelf);
            _urlAspellWin32.create(::GetDlgItem(_hSelf, IDC_ASPELL_WIN_URL), _T("http://aspell.net/win32/"));

			/* Change dialog lang */
			NLChangeDialog(_hInst, _nppData._nppHandle, _hSelf, _T("NotAvailable"));

			return TRUE;
		}
		case WM_COMMAND : 
		{
			switch (wParam)
			{
                case IDCANCEL :
					::EndDialog(_hSelf, 0);
					return TRUE;
				case IDOK :
					::GetDlgItemText(_hSelf, IDC_EDIT_RELPATH, _pSCProp->szRelPath, MAX_PATH);
					::EndDialog(_hSelf, 0);
					return TRUE;

				default :
					break;
			}
		}
	}
	return FALSE;
}

