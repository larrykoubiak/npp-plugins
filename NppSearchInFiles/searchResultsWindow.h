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

class SearchInFilesDock;

//////////////////////////////////////////////////////////////////////////////////////
// This window contains the ListCtrl that shows the search in files results
class searchResultsWindow : public StaticDialog
{
	public:

		bool hasImageList() { return m_bItHasImageList; };

		void initEx(HINSTANCE hInst, HWND parent, SearchInFilesDock* searchInFilesDock) {
			m_searchInFilesDock = searchInFilesDock;
			init(hInst, parent);
		}

		~searchResultsWindow() {
			// Destroy our list font
			if (m_font.m_hFont) m_font.DeleteObject();
		}

		void setSearchLength(UINT searchLength) { m_searchLength = searchLength; };
		UINT getSearchLength() { return m_searchLength; };

		void openCurrSelection(int numItem);

		void destroy() {
			::SendMessage(_hParent, WM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_hSelf);
			::DestroyWindow(_hSelf);
		};

	protected :
		BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);

		void InitTableImageList();
		void InitTableList();

		searchResultsListCtrl	m_searchResultsListCtrl;

		bool					m_bItHasImageList;
		SearchInFilesDock*		m_searchInFilesDock;
		UINT					m_searchLength;
		CFont					m_font;
};
