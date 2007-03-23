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

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "SearchInFilesDlg.h"

BOOL CALLBACK SearchInFilesDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	try {
		switch (message) 
		{
			case WM_INITDIALOG :
				{
					// Leemos el estado inicial
					m_bShowSearchInFiles = (::GetPrivateProfileInt(getSectionName(), getKeyName(), 0, m_iniFilePath) != 0);

					// These flags should be read from configuration
					SIFTabBarPlus::doDragNDrop(true);
					SIFTabBarPlus::setDrawTopBar(true);
					SIFTabBarPlus::setDrawInactiveTab(false);
					SIFTabBarPlus::setDrawTabCloseButton(true);
					SIFTabBarPlus::setDbClk2Close(true);

					// Creamos el control de solapas
					_ctrlTab.init(_hInst, _hSelf, false);
					_ctrlTab.setFont("Tahoma", 13);

					_searchResultsDlg.init(_hInst, _ctrlTab.getHSelf());
					_searchResultsDlg.create(IDD_SEARCH_RESULTS);
					_searchResultsDlg.display();

					_wVector.push_back(DlgInfo(&_searchResultsDlg, "Search Results"));
					_wVector.push_back(DlgInfo(&_searchResultsDlg, "Search Results 2"));

					_ctrlTab.createTabs(_wVector);
					_ctrlTab.display();
					return TRUE;
				}

			case WM_DESTROY:
				::WritePrivateProfileString(getSectionName(), getKeyName(), 
											m_bShowSearchInFiles ? "1" : "0", 
											m_iniFilePath);
				_searchResultsDlg.destroy();
				_ctrlTab.destroy();
				return TRUE;

			case WM_SHOWWINDOW:
				{
					m_bShowSearchInFiles = (BOOL)wParam ? true : false; 
					return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
				}

			case WM_SYSCOMMAND:
				{
					if (wParam == SC_KEYMENU && lParam == 0x71) {
						display(false);
						::SetFocus(_hParent);
						return FALSE;
					}
					else
						return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
				}

			case WM_SIZE: 
				{
					RECT rc, rcResultsDlg;

					getClientRect(rc);

					rcResultsDlg = rc;

					rc.top += 26;
					rc.left += 2;
					rc.right -= 2;
					rc.bottom -= 28;

					_ctrlTab.reSizeTo(rc);

					rcResultsDlg.top += 21;
					rcResultsDlg.left += 1;
					rcResultsDlg.right -= 13;
					rcResultsDlg.bottom -= 58;
					_searchResultsDlg.reSizeTo(rcResultsDlg);
				}
				break; 

				case WM_DRAWITEM :
				{
					DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
					if (dis->CtlType == ODT_TAB)
					{
						return (BOOL)::SendMessage(dis->hwndItem, WM_DRAWITEM, wParam, lParam);
					}
				}

			default :
				return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
		}
	}
	catch (...) {
		::MessageBox(NULL, "Excepción controlada en SearchInFilesDlg::run_dlgProc", "searchInFilesDlg.cpp", MB_OK);
	}
	return FALSE;
}

BOOL CALLBACK SearchResultsDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) {
	return FALSE;
}

