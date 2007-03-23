//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"

#include "lightExplorerDlg.h"

BOOL CALLBACK lightExplorerDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG:
		{
			// Create a font using the system message font
			NONCLIENTMETRICS ncm;

			ncm.cbSize = sizeof(NONCLIENTMETRICS);
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
				m_font.CreateFontIndirect(&(ncm.lfMessageFont));
			else 
				m_font.CreateFontA(-11,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Tahoma");

			// Subclass the tree control
			m_wndTreeCtrl.SubclassWindow(::GetDlgItem(_hSelf, IDC_TREECTRL), m_iniFilePath);
			m_wndTreeCtrl.SetFont(m_font);
			m_wndTreeCtrl.setNppHandles(m_nppHandle, m_scintillaMainHandle);
			return TRUE;
		}

		case WM_SIZE:
		{
			RECT rc;

			::GetClientRect(_hSelf, &rc);
			m_wndTreeCtrl.MoveWindow(rc.left + 2, rc.top + 2, rc.right - 2, rc.bottom - 2, TRUE);
			return TRUE;
		}

		case WM_NOTIFY:
		{
			NMHDR* pNMHDR = (LPNMHDR)lParam;
			BOOL bHandled;

			if (pNMHDR->hwndFrom == ::GetDlgItem(_hSelf, IDC_TREECTRL)) {
				CUTL_BUFFER tempBuf;

				switch (pNMHDR->code) {
					case TVN_ITEMEXPANDING:
						return m_wndTreeCtrl.OnItemExpanding(IDC_TREECTRL, pNMHDR, bHandled);

					case NM_DBLCLK: 
						return m_wndTreeCtrl.OnLButtonDblClick(IDC_TREECTRL, pNMHDR, bHandled);

					case NM_CLICK: 
						return m_wndTreeCtrl.OnItemClick(IDC_TREECTRL, pNMHDR, bHandled);

					case NM_RCLICK:
						return m_wndTreeCtrl.OnRClickItem(IDC_TREECTRL, pNMHDR, bHandled, FALSE);

					case TVN_DELETEITEM:
						return m_wndTreeCtrl.onDeleteItem(IDC_TREECTRL, pNMHDR, bHandled);

					default: 
						return (BOOL)::SendMessage(::GetDlgItem(_hSelf, IDC_TREECTRL), message, wParam, lParam);
				}
			}
			break;
		}

		case WM_SYSCOMMAND:
		{
			// We manage here the ALT+A keyboad
			if (wParam == SC_KEYMENU && lParam == 0x61) {
				display(isVisible() ? false : true);
				if (!isVisible()) 
					::SendMessage(m_scintillaMainHandle, WM_SETFOCUS, (WPARAM)_hSelf, 0L); // Give the focus to notepad++
				return FALSE;
			}
			else
				return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
		}

		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
	return FALSE;
}


