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

#include "dockingFeature/staticDialog.h"
#include "SearchResultsListCtrl.h"
#include "searchResultsWindow.h"

BOOL searchResultsListCtrl::SubclassWindow(HWND hWnd, searchResultsWindow* pSearchResultsWindow) {
	if (CWindowImpl<searchResultsListCtrl, CListViewCtrl>::SubclassWindow(hWnd)) {
		m_searchResultsWindow = pSearchResultsWindow;
		return TRUE;
	}
	return FALSE;
}

BOOL searchResultsListCtrl::DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult) {
	return FALSE;
}

LRESULT searchResultsListCtrl::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	switch(wParam) {
		case VK_F4:
			::SendMessage(m_searchResultsWindow->getHSelf(), uMsg, wParam, lParam);
			break;

		case VK_RETURN:
		case VK_SPACE:
			m_searchResultsWindow->openCurrSelection(GetSelectedIndex());
			break;

		default:
			return 0;
	}
	return 1;
}
