//this file is part of NppSearchInFiles
//Copyright (C)2007 Jose Javier SAnjosé ( dengGB.balandro@gmail.com )
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

#include "window.h"
#include "dockingFeature/staticDialog.h"
#include "tabBar/tabBar.h"

#include "SearchResultsListCtrl.h"
#include "searchResultsWindow.h"
#include "SearchInFilesDock.h"

#pragma warning ( disable : 4311 )

//////////////////////////////////////////////////////////////////////////////////////////////
// searchResultListCtrl class
//////////////////////////////////////////////////////////////////////////////////////////////
void searchResultsWindow::openCurrSelection(int numItem) {
	try {
		HWND listCtrlHWND = ::GetDlgItem(_hSelf, IDC_RESULTSLIST);
		CUTL_BUFFER filePath(MAX_PATH + 1), fileName(MAX_PATH + 1), lineNumber(MAX_PATH + 1), col(MAX_PATH + 1);

		ListView_GetItemText(listCtrlHWND, numItem, 2, (LPSTR)filePath.GetSafe(), MAX_PATH);
		ListView_GetItemText(listCtrlHWND, numItem, 0, (LPSTR)fileName.GetSafe(), MAX_PATH);
		ListView_GetItemText(listCtrlHWND, numItem, 3, (LPSTR)lineNumber.GetSafe(), MAX_PATH);
		ListView_GetItemText(listCtrlHWND, numItem, 4, (LPSTR)col.GetSafe(), MAX_PATH);

		// We select the current selection
		LockWindowUpdate(listCtrlHWND);
		ListView_EnsureVisible(listCtrlHWND, numItem, FALSE);
		ListView_SetItemState(listCtrlHWND, numItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		LockWindowUpdate(NULL);

		CUTL_BUFFER fileToOpen;

		::SendMessage(m_searchInFilesDock->m_nppHandle, WM_DOOPEN, 0, (LPARAM)(LPSTR)fileToOpen.Sf("%s\\%s", filePath.GetSafe(), fileName.GetSafe()));

		int startPos = (int)::SendMessage(m_searchInFilesDock->m_scintillaMainHandle, SCI_POSITIONFROMLINE, atoi(lineNumber.Trim().GetSafe()) - 1, 0L);

		startPos += atoi(col.Trim().GetSafe()) - 1;
		::SendMessage(m_searchInFilesDock->m_scintillaMainHandle, SCI_SETSEL, startPos, startPos + getSearchLength());
	}
	catch(...) {
		systemMessageEx("Error at searchResultListCtrl::openCurrSelection", __FILE__, __LINE__);
	}
}

BOOL CALLBACK searchResultsWindow::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	try 
	{
		switch (message) 
		{
			case WM_INITDIALOG:
				{
					m_bItHasImageList = false;

					// SubClass the ListCtrl
					m_searchResultsListCtrl.SubclassWindow(::GetDlgItem(_hSelf, IDC_RESULTSLIST), this);

					// Create a font using the system message font
					NONCLIENTMETRICS ncm;

					ncm.cbSize = sizeof(NONCLIENTMETRICS);
					if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
						m_font.CreateFontIndirect(&(ncm.lfMessageFont));
					else 
						m_font.CreateFontA(-11,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Tahoma");

					m_searchResultsListCtrl.SetFont(m_font);
					m_searchResultsListCtrl.SetExtendedListViewStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, 
											 LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

					InitTableImageList();
					InitTableList();
					return TRUE;
				}

			case WM_SIZE:
				{
					RECT rc, rcStatic;

					getClientRect(rc);

					rcStatic = rc;

					rcStatic.left += 8;
					rcStatic.bottom = rcStatic.top + 18;
					::MoveWindow(::GetDlgItem(_hSelf, IDC_STATIC_STATUS), rcStatic.left, rcStatic.top, rcStatic.right, rcStatic.bottom, TRUE);

					rc.top += 20;
					rc.left += 2;
					rc.bottom -=20;
					rc.right -=2;
					m_searchResultsListCtrl.MoveWindow(rc.left, rc.top, rc.right, rc.bottom, TRUE);
					return TRUE;
				}

			case WM_NOTIFY:
				{
					SCNotification *scn = (SCNotification *)lParam;

					if (scn->nmhdr.code == NM_DBLCLK) {
						NMITEMACTIVATE* lpnmia = (LPNMITEMACTIVATE)lParam;
						openCurrSelection(lpnmia->iItem);
					}
				}
				break;

			case WM_KEYUP:
				if (wParam == VK_F4) m_searchInFilesDock->moveToNextHit();
				break;


			default:
				return FALSE;
		}
	}
	catch (...) {
		systemMessageEx("Error at searchResultListCtrl::run_dlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

void searchResultsWindow::InitTableImageList()
{
	try {
		OSVERSIONINFO osVer = { sizeof(OSVERSIONINFO), 0L, 0L, 0L, 0L, "\0"};

		::GetVersionEx(&osVer);
   
		HIMAGELIST hImageList = NULL;
		if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) {
			SHFILEINFO sfi;
			memset(&sfi, 0, sizeof(sfi));
			HIMAGELIST hil = reinterpret_cast<HIMAGELIST> (SHGetFileInfo ("C:\\", 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
			if (hil) {
				m_searchResultsListCtrl.SetImageList(hil, LVSIL_SMALL);
				m_bItHasImageList = true;
			}
		} 
	}
	catch (...) {
		systemMessageEx("Error at searchResultListCtrl::InitTableImageList", __FILE__, __LINE__);
	}
}

void searchResultsWindow::InitTableList()
{
	HWND hTableList = GetDlgItem(_hSelf, IDC_RESULTSLIST);

	LVCOLUMN listCol;
	memset(&listCol, 0, sizeof(LVCOLUMN));
	listCol.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	listCol.pszText = "File Name";
	listCol.cx = 0x50;
	// Insert columns
	m_searchResultsListCtrl.InsertColumn(0, &listCol);

	listCol.cx = 0x42;
	listCol.pszText="Text";                            
	m_searchResultsListCtrl.InsertColumn(1, &listCol);

	listCol.pszText="Folder";                            
	m_searchResultsListCtrl.InsertColumn(2, &listCol);

	listCol.pszText="Line";
	listCol.mask |= LVCF_FMT;
	listCol.fmt = LVCFMT_RIGHT;
	m_searchResultsListCtrl.InsertColumn(3, &listCol);

	listCol.pszText="Column";                            
	m_searchResultsListCtrl.InsertColumn(4, &listCol);

	listCol.cx = 0x62;
	listCol.pszText="Date Modified";
	listCol.fmt = LVCFMT_LEFT;
	m_searchResultsListCtrl.InsertColumn(5, &listCol);
}
