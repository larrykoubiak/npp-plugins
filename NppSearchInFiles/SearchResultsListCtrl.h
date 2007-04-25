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

class SearchInFilesDock;

class CCustomItemInfo {
public:
	CCustomItemInfo(int line, int column, LPCSTR fullPath) : m_line(line), m_column(column), m_fullPath(fullPath) {};

public:
	int			m_line;
	int			m_column;
	CUTL_BUFFER m_fullPath;
};

class SearchResultsTreeCtrl : public CWindowImpl<SearchResultsTreeCtrl, CTreeViewCtrl>
{
public:
	SearchResultsTreeCtrl() : m_firstItem(NULL) {};

	BOOL SubclassWindow(HWND hWnd, SearchInFilesDock* pSearchInFilesDock);

	~SearchResultsTreeCtrl() {}

	BEGIN_MSG_MAP(searchResultsListCtrl)
		MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
		DEFAULT_REFLECTION_HANDLER()
	END_MSG_MAP()

	LRESULT OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	bool onDeleteItem(LPNMHDR pnmh);
	bool OnRClickItem(LPNMHDR pnmh);

	BOOL DefaultReflectionHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult);

	bool hasImageList() { return m_bItHasImageList; };

	void		SetFirstItem(HTREEITEM firstItem)	{ m_firstItem = firstItem; };
	HTREEITEM	GetFirstItem()						{ return m_firstItem; };

private:
	SearchInFilesDock*		m_searchInFilesDock;
	bool					m_bItHasImageList;
	HTREEITEM				m_firstItem;

	void InitTableImageList();
	void InitTableList();
};