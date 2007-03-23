//this file is part of notepad++ plug in 'Search In Files'
//Copyright (C)2007 Jose J Sanjosé ( dengGB.balandro@gmail.com )
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


class CProcessSearchInFiles
{
public:
	CProcessSearchInFiles(SearchInFilesDock* searchDock, HWND searchInputDlgHnd): m_searchDock(searchDock), m_searchInputDlgHnd(searchInputDlgHnd)
	{
		m_searchDockStaticHWND	= ::GetDlgItem(m_searchDock->getCurrentSearchResultsDialog()->getHSelf(), IDC_STATIC_STATUS);
		m_searchDockListHWND	= ::GetDlgItem(m_searchDock->getCurrentSearchResultsDialog()->getHSelf(), IDC_RESULTSLIST);
	};

	void doSearch();

protected:

	SearchInFilesDock*	m_searchDock;
	HWND				m_searchInputDlgHnd;
	HWND				m_searchDockStaticHWND;
	HWND				m_searchDockListHWND;

	CUTL_BUFFER			m_foldersArray;
	int					m_totalHits;
	int					m_totalFolders;
	int					m_totalFiles;

	bool				m_bCaseSens;
	bool				m_bWholeWord;
	bool				m_bExcludeExtensions;
	CUTL_BUFFER			m_excludeExtensionsList;
	CUTL_BUFFER			m_percentageProgress;

	bool checkCancelButton(); // Verifies if the cancel button was pressed
	void ResizeInputDgl();
	bool SearchFolders(LPCSTR folder);
	bool FindInfile(LPCSTR file);
	int  FileSize(const char * szFileName);
	bool FindInLine(LPCSTR StringLine, LPCSTR lineToShow, LPCSTR searchPattern, CUTL_PATH iterator, int line);
	bool SearchInFolders();
	void doFixedColumnsResize();
};