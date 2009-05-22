/*
This file is part of Function List Plugin for Notepad++
Copyright (C)2005-2007 Jens Lorenz <jens.plugin.npp@gmx.de>

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

#include "IconSelectDialog.h"
#include <commctrl.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shellapi.h>


UINT IconSelectDialog::doDialog(HIMAGELIST hIml, LPINT piImg)
{
	_hIml			= hIml;
	_piImg			= piImg;
	return (UINT)::DialogBoxParam(_hInst, MAKEINTRESOURCE(IDD_ICON_DLG), NULL, (DLGPROC)dlgProc, (LPARAM)this);
}

BOOL IconSelectDialog::run_dlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_INITDIALOG:
		{
			RECT	rcWnd		= {0};
			RECT	rcParent	= {0};

			/* set to cursor position */
			::GetWindowRect(_hSelf, &rcWnd);
			::GetWindowRect(_hParent, &rcParent);
			::MoveWindow(_hSelf, rcParent.right - rcWnd.right, rcParent.bottom, rcWnd.right, rcWnd.bottom, TRUE);

			/* get handle and create subclass */
			_hListCtrl = ::GetDlgItem(_hSelf, IDC_LIST_ICONSEL);
			::SetWindowLong(_hListCtrl, GWL_USERDATA, (LONG)this);
			_hDefaultListProc = (WNDPROC)::SetWindowLong(_hListCtrl, GWL_WNDPROC, (LONG)wndListProc);

			/* create 1 columns for 16 elements */
			LVCOLUMN	clm = {0};
			clm.mask	= LVCF_WIDTH;
			clm.cx		= 256;
			ListView_InsertColumn(_hListCtrl, 0, &clm);

			/* set item count */
			ListView_SetImageList(_hListCtrl, _hIml, LVSIL_SMALL);
			ListView_SetItemCountEx(_hListCtrl, 1 + ImageList_GetImageCount(_hIml) / 16, LVSICF_NOSCROLL);

			/* get visible line count */
			_uMaxImages = (1 + ListView_GetCountPerPage(_hListCtrl)) * 16;

			/* view current line */
			ListView_EnsureVisible(_hListCtrl, *_piImg / 16, TRUE);
			break;
		}
		case WM_ACTIVATE:
		{
			if (wParam == WA_INACTIVE)
			{
				::EndDialog(_hSelf, FALSE);
				return TRUE;
			}
			break;
		}
		default:
			break;
	}

	return FALSE; 
}

LRESULT IconSelectDialog::runProcList(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_LBUTTONDOWN:
		{
			/* get selected icon */
			UINT uIcon = ListView_GetTopIndex(_hListCtrl) * 16;
			*_piImg = (LOWORD(lParam) >> 4) + (HIWORD(lParam) & 0xFFF0) + uIcon;
			::EndDialog(_hSelf, TRUE);
			return TRUE;
		}
		case WM_PAINT:
		{
			RECT	rc			= {0};
			HDC		hDc			= ::GetWindowDC(hwnd);
			LRESULT lpRet		= ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
			UINT	cnt			= ImageList_GetImageCount(_hIml);
			UINT	uIconOffset	= ListView_GetTopIndex(_hListCtrl) * 16;

			::GetClientRect(_hListCtrl, &rc);

			for (UINT i = 0; (i < cnt) && (i < _uMaxImages); i++)
			{
				ImageList_Draw(_hIml,
					uIconOffset + i,
					hDc, 
					1 + 16 * (i % 16), 
					1 + 16 * (i / 16), 
					ILD_NORMAL);
			}

			UINT uFocus	= *_piImg - uIconOffset;
			rc.left		= 1 + 16 * (uFocus % 16);
			rc.top		= 1 + 16 * (uFocus / 16);
			rc.right	= rc.left + 16;
			rc.bottom	= rc.top  + 16;
			::DrawFocusRect(hDc, &rc);
			return lpRet;
		}
		default:
			break;
	}

	return ::CallWindowProc(_hDefaultListProc, hwnd, Message, wParam, lParam);
}



