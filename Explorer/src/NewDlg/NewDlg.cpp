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

#include "NewDlg.h"
#include "stdio.h"


UINT NewDlg::doDialog(LPTSTR pFileName, LPTSTR pDesc)
{
	_pFileName = pFileName;
	_pDesc = pDesc;
	return ::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_NEW_DLG), _hParent,  (DLGPROC)dlgProc, (LPARAM)this);
}


BOOL CALLBACK NewDlg::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) 
	{
		case WM_INITDIALOG:
		{
			TCHAR	szDesc[MAX_PATH];

			if (_pszWndName != NULL)
				::SetWindowText(_hSelf, _pszWndName);

			sprintf(szDesc, "%s:", _pDesc);
			::SetWindowText(::GetDlgItem(_hSelf, IDC_STATIC_NEW_DESC), szDesc);

			::SetWindowText(::GetDlgItem(_hSelf, IDC_EDIT_NEW), _pFileName);

			goToCenter();
			SetFocus(::GetDlgItem(_hSelf, IDC_EDIT_NEW));
			break;
		}
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					::EndDialog(_hSelf, FALSE);
					return TRUE;
				case IDOK:
				{
					UINT	length	= ::SendDlgItemMessage(_hSelf, IDC_EDIT_NEW, WM_GETTEXTLENGTH, 0, 0) + 1;

					SendDlgItemMessage(_hSelf, IDC_EDIT_NEW, WM_GETTEXT, length, (LPARAM)_pFileName);
					::EndDialog(_hSelf, TRUE);
					return TRUE;
				}
				default:
					break;
			}
			break;
		}
		case WM_DESTROY :
		{
			/* deregister this dialog */
			::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (LPARAM)_hSelf);
			break;
		}
		default:
			break;
	}
	return FALSE;
}



