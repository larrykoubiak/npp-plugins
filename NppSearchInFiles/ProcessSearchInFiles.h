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
	CProcessSearchInFiles(SearchInFilesDock* searchDock, SearchInFilesDock* mainDock, HWND searchInputDlgHnd): m_searchDock(searchDock), m_mainDock(mainDock), m_searchInputDlgHnd(searchInputDlgHnd)
	{
		m_pSearchDockList = m_searchDock->getResultsTree();
	};

	void doSearch();

protected:

	SearchInFilesDock*		m_mainDock;
	SearchInFilesDock*		m_searchDock;
	SearchResultsTreeCtrl*	m_pSearchDockList;
	HWND					m_searchInputDlgHnd;

	CUTL_BUFFER			m_foldersArray;
	int					m_totalHits;
	int					m_totalFolders;
	int					m_totalFiles;
	int					m_currFileHits;
	int					m_currFileIcon;

	bool				m_bCaseSens;
	bool				m_bWholeWord;
	bool				m_bExcludeExtensions;
	CUTL_BUFFER			m_excludeExtensionsList;
	CUTL_BUFFER			m_percentageProgress;
	CUTL_PATH			m_currHitFile;
	HTREEITEM			m_currRootItem;

	bool checkCancelButton(); // Verifies if the cancel button was pressed
	void ResizeInputDgl();
	bool SearchFolders(LPCSTR folder);
	bool FindInfile(LPCSTR file);
	int  FileSize(const char * szFileName);
	BOOL DoFind(LPCSTR strLine, LPCSTR searchPattern, UINT& hitpos, UINT endPosLine, BOOL& wholeWordSucess);
	bool FindInLine(LPCSTR StringLine, LPCSTR lineToShow, LPCSTR searchPattern, CUTL_PATH iterator, int line);
	bool SearchInFolders();

	int  GetIconIndex(const CUTL_BUFFER sFilename);
	int  GetSelIconIndex(const CUTL_BUFFER sFilename);
};