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

class SearchResultsListCtrl;

#include "dockingFeature/staticDialog.h"
#include "SearchResultsListCtrl.h"
#include "SearchInFilesDock.h"
#include "misc/SysMsg/SysMsg.h"

BOOL SearchResultsListCtrl::SubclassWindow(HWND hWnd, SearchInFilesDock* pSearchInFilesDock) {
	if (CWindowImpl<SearchResultsListCtrl, CTreeViewCtrl>::SubclassWindow(hWnd)) {
		m_searchInFilesDock = pSearchInFilesDock;

		m_bItHasImageList = false;

		SetExtendedStyle(LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, 
		 			     LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

		InitTableImageList();
		//InitTableList();
		return TRUE;
	}
	return FALSE;
}

BOOL SearchResultsListCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
	switch (uMsg) 
	{
		case WM_LBUTTONDBLCLK:
			m_searchInFilesDock->openCurrSelection(GetSelectedItem());
			break;

		default:
			break;
	}
	return FALSE;
}

LRESULT SearchResultsListCtrl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	switch(wParam) {
		case VK_F4:
			m_searchInFilesDock->moveToNextHit();
			break;

		case VK_RETURN:
		case VK_SPACE:
			m_searchInFilesDock->openCurrSelection(GetSelectedItem());
			break;

		default:
			return 0;
	}
	return 1;
}

void SearchResultsListCtrl::InitTableImageList()
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
				SetImageList(hil, LVSIL_SMALL);
				m_bItHasImageList = true;
			}
		} 
	}
	catch (...) {
		systemMessageEx("Error at searchResultListCtrl::InitTableImageList", __FILE__, __LINE__);
	}
}

/*
void SearchResultsListCtrl::InitTableList()
{
	LVCOLUMN listCol;
	memset(&listCol, 0, sizeof(LVCOLUMN));
	listCol.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
	listCol.pszText = "File Name";
	listCol.cx = 0x50;
	// Insert columns
	InsertColumn(0, &listCol);

	listCol.cx = 0x42;
	listCol.pszText="Text";                            
	InsertColumn(1, &listCol);

	listCol.pszText="Folder";                            
	InsertColumn(2, &listCol);

	listCol.pszText="Line";
	listCol.mask |= LVCF_FMT;
	listCol.fmt = LVCFMT_RIGHT;
	InsertColumn(3, &listCol);

	listCol.pszText="Column";                            
	InsertColumn(4, &listCol);

	listCol.cx = 0x62;
	listCol.pszText="Date Modified";
	listCol.fmt = LVCFMT_LEFT;
	InsertColumn(5, &listCol);
}
*/