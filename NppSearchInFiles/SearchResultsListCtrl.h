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

#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlmisc.h>

class searchResultsWindow;

class searchResultsListCtrl : public CWindowImpl<searchResultsListCtrl, CListViewCtrl>
{
public:
	BOOL SubclassWindow(HWND hWnd, searchResultsWindow* pSearchResultsWindow);

	BEGIN_MSG_MAP(searchResultsListCtrl)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	BOOL DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

private:
	searchResultsWindow*		m_searchResultsWindow;
};