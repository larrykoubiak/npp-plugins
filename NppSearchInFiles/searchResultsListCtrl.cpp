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

#define COLAPSE_ALL  2001
#define EXPAND_ALL	 2002

BOOL SearchResultsTreeCtrl::SubclassWindow(HWND hWnd, SearchInFilesDock* pSearchInFilesDock) {
	if (CWindowImpl<SearchResultsTreeCtrl, CTreeViewCtrl>::SubclassWindow(hWnd)) {
		m_searchInFilesDock = pSearchInFilesDock;

		m_bItHasImageList = false;

		InitTableImageList();
		return TRUE;
	}
	return FALSE;
}

BOOL SearchResultsTreeCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
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

LRESULT SearchResultsTreeCtrl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	switch(wParam) {
		case VK_F4:
			m_searchInFilesDock->moveToNextHit();
			break;

		case VK_RETURN:
		case VK_SPACE:
			m_searchInFilesDock->openCurrSelection(GetSelectedItem());
			break;

		case VK_TAB:
		case VK_ESCAPE:
			::SendMessage(m_searchInFilesDock->m_scintillaMainHandle, WM_SETFOCUS, (WPARAM)m_hWnd, 0L); // Give the focus to notepad++
			break;

		default:
			return 0;
	}
	return 1;
}

void SearchResultsTreeCtrl::InitTableImageList()
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
				SetImageList(hil, TVSIL_NORMAL);
				m_bItHasImageList = true;
			}
		} 
	}
	catch (...) {
		systemMessageEx("Error at searchResultListCtrl::InitTableImageList", __FILE__, __LINE__);
	}
}

bool SearchResultsTreeCtrl::onDeleteItem(LPNMHDR pnmh) {
	try {
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pnmh;

		CCustomItemInfo* cii = (CCustomItemInfo*)pNMTreeView->itemOld.lParam;

		// Let's free the memory
		if (cii) delete cii;
	}
	catch (...) {
		systemMessage("Error at SearchResultsListCtrl::onDeleteItem.");
	}
	return 0;
}

bool  SearchResultsTreeCtrl::OnRClickItem(LPNMHDR pnmh) {
	try {
		POINT	screenPoint, clientPoint;
		UINT	uFlags;

		::GetCursorPos(&screenPoint);

		clientPoint = screenPoint;

		ScreenToClient(&clientPoint);

		HTREEITEM hHitItem = HitTest(clientPoint, &uFlags);

		// The rClick was over a tree item?
		if (hHitItem != NULL) SelectItem(hHitItem); 

		// And now the context menu
		HMENU hPopupMenu = ::CreatePopupMenu();

		MENUITEMINFO mii;

		memset(&mii, 0, sizeof(mii));
		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_STRING | MIIM_DATA | MIIM_STATE | MIIM_ID;

		// Collapse all
		mii.wID			= COLAPSE_ALL;
		mii.dwTypeData	= "Colapse All";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= GetCount() ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, COLAPSE_ALL, FALSE, &mii);

		// Expand all
		mii.wID			= EXPAND_ALL;
		mii.dwTypeData	= "Expand All";
		mii.cch			= UTL_strlen(mii.dwTypeData);
		mii.fState		= GetCount() ? MFS_ENABLED : MFS_DISABLED;

		::InsertMenuItem(hPopupMenu, EXPAND_ALL, FALSE, &mii);

		// Let's open the context menu
		int resp = ::TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN | TPM_RETURNCMD , screenPoint.x, screenPoint.y, 0, m_hWnd, NULL);

		if (resp == COLAPSE_ALL || resp == EXPAND_ALL) {
			CWaitCursor	wCursor;
			HTREEITEM	hCurrent = GetFirstItem();
			while (hCurrent != NULL)
			{
				Expand(hCurrent, resp == COLAPSE_ALL ? TVE_COLLAPSE : TVE_EXPAND);

			   // Try to get the next item
			   hCurrent = GetNextItem(hCurrent, TVGN_NEXT);
			}

			// Show the current selection
			if (GetSelectedItem()) EnsureVisible(GetSelectedItem());
		}
	}
	catch (...) {
		systemMessage("Error at SearchResultsListCtrl::onDeleteItem.");
	}
	return 0;
}
