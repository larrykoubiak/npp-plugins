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

#include "stdafx.h"

#include "dockingFeature/staticDialog.h"
#include "tabBar/tabBar.h"
#include "SearchResultsListCtrl.h"
#include "searchResultsWindow.h"
#include "SearchInFilesDock.h"
#include "ProcessSearchInFiles.h"

#pragma warning ( disable : 4311 )

//////////////////////////////////////////////////////////////////////////////////////////////
// SearchInFilesDock class
//////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK SearchInFilesDock::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	try {
		switch (message) 
		{
			case WM_INITDIALOG:
			{
				// Leemos el estado inicial
				m_bShowSearchInFiles = (::GetPrivateProfileInt(getSectionName(), getKeyName(), 0, m_iniFilePath) != 0);

				// Creamos el control de solapas
				_ctrlTabIconList.create(_hInst, 16);

				_ctrlTabIconList.addImage(IDR_SEARCH);

				_ctrlTab.init(_hInst, _hSelf);
				_ctrlTab.setImageList(_ctrlTabIconList.getHandle());
				_ctrlTab.setFont("Tahoma", 13);

				_searchResultsDlg.initEx(_hInst, _ctrlTab.getHSelf(), this);
				_searchResultsDlg.create(IDD_SEARCH_RESULTS);
				_searchResultsDlg.display();

				m_searchResultsDlgVector.push_back(&_searchResultsDlg);

				_wVector.push_back(DlgInfo(&_searchResultsDlg, "Search Results"));

				_ctrlTab.createTabs(_wVector);
				_ctrlTab.display();

				// These flags should be read from configuration
				SIFTabBarPlus::doDragNDrop(false);
				SIFTabBarPlus::setDrawTopBar(true);
				SIFTabBarPlus::setDrawInactiveTab(false);
				SIFTabBarPlus::setDrawTabCloseButton(true);
				SIFTabBarPlus::setDbClk2Close(true);
				return TRUE;
			}

			case WM_DESTROY:
			{
				::WritePrivateProfileString(getSectionName(), getKeyName(), 
											m_bShowSearchInFiles ? "1" : "0", 
											m_iniFilePath);
				_searchResultsDlg.destroy();
				_ctrlTab.destroy();
				return TRUE;
			}

			case WM_SHOWWINDOW:
			{
				m_bShowSearchInFiles = (BOOL)wParam ? true : false; 
				return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
			}

			case WM_SYSCOMMAND:
			{
				// We manage here the ALT+Q keyboad
				if (wParam == SC_KEYMENU && lParam == 0x71) {
					display();
					openSearchInFilesInputDlg();
					return true;
				}
				else
					return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
			}

			case WM_SIZE: 
			{
				HWND currListCtrl = ::GetDlgItem(getCurrentSearchResultsDialog()->getHSelf(), IDC_RESULTSLIST); 

				::SendMessage(currListCtrl, WM_SETREDRAW, FALSE, 0L);

				RECT rc, rcResultsDlg;

				getClientRect(rc);

				rcResultsDlg = rc;

				rc.top += 0;
				rc.left += 2;
				rc.right -= 2;
				rc.bottom -= 2;

				_ctrlTab.reSizeTo(rc);

				rcResultsDlg.top += 30;
				rcResultsDlg.left += 1;
				rcResultsDlg.right -= 13;
				rcResultsDlg.bottom -= 42;

				for (UINT i = 0; i < m_searchResultsDlgVector.size(); i++) {
					searchResultsWindow* srd = m_searchResultsDlgVector[i];

					if (i == _ctrlTab.getCurrentTab())
						srd->reSizeTo(rcResultsDlg);
					else
						srd->display(false);
				}
				::SendMessage(currListCtrl, WM_SETREDRAW, TRUE, 0L);
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
			break;

			case WM_NOTIFY:		  
			{
				NMHDR *nmhdr = (NMHDR *)lParam;
				
				switch (nmhdr->code) 
				{
					case TCN_SELCHANGE:
						if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
						{
							_ctrlTab.clickedUpdate();
							::SendMessage(_hSelf, WM_SIZE, (WPARAM)SIZE_RESTORED, 0L);
							return TRUE;
						}
						break;

					case TCN_TABDELETE:
						if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
						{
							int currTab = _ctrlTab.getCurrentTab();

							if (currTab >= 1) {
								_ctrlTab.deletItemAt(currTab);
								_ctrlTab.activateWindowAt(currTab - 1);
								_ctrlTab.activateAt(currTab - 1);

								// Delete the LIst Control
								searchResultsWindow* srDlg = m_searchResultsDlgVector.at(currTab);
								SearchResultsDlgVector::iterator srdIterator = m_searchResultsDlgVector.begin() + currTab;
								m_searchResultsDlgVector.erase(srdIterator);

								std::vector<DlgInfo>::iterator hDlg = _wVector.begin() + currTab;
								_wVector.erase(hDlg);

								srDlg->destroy();
								delete srDlg; // Finaly delete the results dialog
								return NULL;
							}
						}
						break;
	
					case TCN_TABDELETE_OTHER:
						if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
						{
							CUTL_BUFFER tempBuf;

							// ¿How may tabs are there?
							int iTabCount = TabCtrl_GetItemCount(_ctrlTab.getHSelf());

							// Place focus over the target tab if it's not alredy the current tab
							if ((UINT)nmhdr->idFrom != TabCtrl_GetCurSel(_ctrlTab.getHSelf())) {
								_ctrlTab.activateWindowAt((UINT)nmhdr->idFrom);
								_ctrlTab.activateAt((UINT)nmhdr->idFrom);
							}

							for (int i = iTabCount - 1; i > 0; i--) {
								if (nmhdr->idFrom == i) continue;

								_ctrlTab.deletItemAt(i);

								// Delete the LIst Control
								searchResultsWindow* srDlg = m_searchResultsDlgVector.at(i);
								SearchResultsDlgVector::iterator srdIterator = m_searchResultsDlgVector.begin() + i;
								m_searchResultsDlgVector.erase(srdIterator);

								std::vector<DlgInfo>::iterator hDlg = _wVector.begin() + i;
								_wVector.erase(hDlg);

								srDlg->destroy();
								delete srDlg; // Finaly delete the results dialog
							}

							//_ctrlTab.activateWindowAt(1);
							//_ctrlTab.activateAt(1);
							return NULL;
						}
						break;

					case TCN_TABDELETE_ALL:
						if (nmhdr->hwndFrom == _ctrlTab.getHSelf())
						{
							CUTL_BUFFER tempBuf;

							// ¿How may tabs are there?
							int iTabCount = TabCtrl_GetItemCount(_ctrlTab.getHSelf());

							// Place focus over the the first tab
							_ctrlTab.activateWindowAt(0);
							_ctrlTab.activateAt(0);

							// Delete them all from last to fist
							for (int i = iTabCount - 1; i > 0; i--) {
								_ctrlTab.deletItemAt(i);

								// Delete the LIst Control
								searchResultsWindow* srDlg = m_searchResultsDlgVector.at(i);
								SearchResultsDlgVector::iterator srdIterator = m_searchResultsDlgVector.begin() + i;
								m_searchResultsDlgVector.erase(srdIterator);

								std::vector<DlgInfo>::iterator hDlg = _wVector.begin() + i;
								_wVector.erase(hDlg);

								srDlg->destroy();
								delete srDlg; // Finaly delete the results dialog
							}
							return NULL;
						}
						break;

					default:
						break;
				}
				break;
			}
			break;
			
			case WM_COMMAND:
			{
				switch(LOWORD(wParam)) {
					case IDM_VIEW_REFRESHTABAR:
						::SendMessage(_hSelf, WM_SIZE, (WPARAM)SIZE_RESTORED, 1L);
						break;

					default:
						break;

				}
				return TRUE;
			}

			case WM_NPPSEARCHINFILES_DOSEARCH_FROM_FOLDER:
			{
				::MessageBox(NULL, "Hola", "WM_NPPSEARCHINFILES_DOSEARCH_FROM_FOLDER", MB_OK);
				break;
			}

			default :
				return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
		}
	}
	catch (...) {
		systemMessageEx("Excepción controlada en _searchInFilesDock::run_dlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

void SearchInFilesDock::openSearchInFilesInputDlg() 
{
	DialogBoxParam(_hInst, 
					(LPCTSTR)IDD_SEARCH_INPUT_DLG, 
					_hParent, 
					(DLGPROC)SearchInputDlg::SearchInFilesInputDlgProc,
					(LPARAM)this);
}

void SearchInFilesDock::callSearchInFiles(HWND hDlg, CUTL_BUFFER what, CUTL_BUFFER types, CUTL_BUFFER where) {
	// Read checks
	bool bWholeWord		  = (::SendMessage(::GetDlgItem(hDlg, IDC_WHOLE_WORD), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? true : false;
	bool bResultsInNewTab = (::SendMessage(::GetDlgItem(hDlg, IDC_RESULTS_IN_NEW_TAB), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? true : false;

	// ¿Create a new tab to show the results? (only if the first tab was alredy used and the user said so)
	CUTL_BUFFER tabText(MAX_PATH), temp;
	TCITEM		tcItem; 

	tcItem.mask			= TCIF_TEXT;
	tcItem.pszText		= (LPSTR)tabText.GetSafe();
	tcItem.cchTextMax	= MAX_PATH-1;

	_CRVERIFY(TabCtrl_GetItem(_ctrlTab.getHSelf(), _ctrlTab.getCurrentTab(), &tcItem));

	int numTabs = TabCtrl_GetItemCount(_ctrlTab.getHSelf());

	temp.Sf("%s   %s   %s", what.GetSafe(), types.GetSafe(), where.GetSafe());

	if (!bResultsInNewTab) {
		_ctrlTab.renameTab(_ctrlTab.getCurrentTab(), temp.GetSafe());
		getCurrentSearchResultsDialog()->setSearchLength(bWholeWord ? what.Len() + 2 : what.Len());
	}
	else {
		if (numTabs == 1 && tabText == "Search Results") {
			_ctrlTab.renameTab(_ctrlTab.getCurrentTab(), temp.GetSafe());
			// We keep the current search length
			_searchResultsDlg.setSearchLength(bWholeWord ? what.Len() + 2 : what.Len());
		}
		else {
			searchResultsWindow* srDlg = new searchResultsWindow();
				
			srDlg->initEx(_hInst, _ctrlTab.getHSelf(), this);
			srDlg->create(IDD_SEARCH_RESULTS);
			srDlg->display();

			m_searchResultsDlgVector.push_back(srDlg);

			_wVector.push_back(DlgInfo(srDlg, (LPSTR)temp.GetSafe()));
			_ctrlTab.activateAt(_ctrlTab.insertAtEnd((LPSTR)temp.GetSafe()));
			_ctrlTab.clickedUpdate();

			srDlg->setSearchLength(bWholeWord ? what.Len() + 2 : what.Len());

			::SendMessage(_hSelf, WM_SIZE, (WPARAM)SIZE_RESTORED, 0L);
		}
	}

	// Finally we do the search
	CProcessSearchInFiles* searchInFiles = new CProcessSearchInFiles(this, hDlg);

	searchInFiles->doSearch();
	delete searchInFiles;
}

void SearchInFilesDock::chooseFolder(HWND hDlg) {
	try {
		LPMALLOC pShellMalloc = 0;
		if (::SHGetMalloc(&pShellMalloc) == NO_ERROR) {
			// If we were able to get the shell malloc object,
			// then proceed by initializing the BROWSEINFO stuct
			BROWSEINFO info;
			char szDisplayName[MAX_PATH];
			CUTL_BUFFER title = "Select a folder to search from";

			memset(&info, 0, sizeof(info));

			info.hwndOwner		= hDlg;
			info.pidlRoot		= NULL;
			info.pszDisplayName = szDisplayName;
			info.lpszTitle		= title.GetSafe();
			info.ulFlags		= 0;
			info.lpfn			= NULL;
			info.lParam			= 0;

			// Execute the browsing dialog.
			LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);

			// pidl will be null if they cancel the browse dialog.
			// pidl will be not null when they select a folder.
			if (pidl) 
			{
				// Try to convert the pidl to a display string.
				// Return is true if success.
				char szDir[MAX_PATH];

				// Set edit control to the directory path.
				if (::SHGetPathFromIDList(pidl, szDir)) {
					::SendMessage(::GetDlgItem(hDlg, IDC_WHERE), CB_INSERTSTRING, 0, (LPARAM)szDir);
					::SendMessage(::GetDlgItem(hDlg, IDC_WHERE), CB_SETCURSEL, 0, 0L);
					::SendMessage(::GetDlgItem(hDlg, IDC_WHERE), WM_SETFOCUS, 0, 0L);
				}
				pShellMalloc->Free(pidl);
			}
			pShellMalloc->Release();
		}
	} 
	catch (...) {
		systemMessageEx("Error in SearchInFilesDock::chooseFolder", __FILE__, __LINE__);
	}
}

void SearchInFilesDock::moveToNextHit() {
	try {
		HWND listCtrlHWND = ::GetDlgItem(getCurrentSearchResultsDialog()->getHSelf(), IDC_RESULTSLIST);

		int itemCount, iSelect;

		if (0 == (itemCount = ListView_GetItemCount(listCtrlHWND)))  return;

		if (-1 == (iSelect = ListView_GetNextItem(listCtrlHWND, -1, LVNI_FOCUSED)))
			iSelect = 0;
		else
			iSelect++;

		iSelect = itemCount <= iSelect ? 0 : iSelect; // If at end, start all over

		getCurrentSearchResultsDialog()->openCurrSelection(iSelect);
	}
	catch (...) {
		systemMessageEx("Error at SearchInFilesDock::moveToNextHit.", __FILE__, __LINE__);
	}
}

void SearchInFilesDock::SaveCombosStrings(HWND hDlg) {
	try {
		// Save the search for futures uses
		CUT2_INI	confIni(m_iniFilePath);
		CUTL_BUFFER inputBuf, outputBuf, savedStrings, temp, counter, matchString;
		INT			i, length;
		UINT		found;

		// what. Save search patterns: each one in a different entry
		confIni.Delete("what");
		i = 0;

		// First Read current selection
		temp.Realloc(254 + 1);
		::GetWindowText(::GetDlgItem(hDlg, IDC_WHAT), temp.data, 254);

		if (temp.Len()) {
			confIni.Write("what", counter.Sf("%d", i), temp.GetSafe());
			savedStrings.Sf("-|-%s-|-", temp.GetSafe());
		}
		else
			savedStrings = "";

		while((length = (INT)::SendMessage(::GetDlgItem(hDlg, IDC_WHAT), CB_GETLBTEXTLEN, (WPARAM)i, 0L)) != CB_ERR) {
			temp.Realloc(length + 1);
			::SendMessage(::GetDlgItem(hDlg, IDC_WHAT), CB_GETLBTEXT, (WPARAM)i++, (LPARAM)temp.GetSafe());

			if (!savedStrings.Find(matchString.Sf("-|-%s-|-", temp.GetSafe()), found)) {
				confIni.Write("what", counter.Sf("%d", i), temp.GetSafe());
				savedStrings.Cat("-|-");
				savedStrings.Cat(temp.GetSafe());
				savedStrings.Cat("-|-");
			}
		}

		// file type. Save files masks: each one in a different entry
		confIni.Delete("masks");
		i = 0;

		// First Read current selection
		temp.Realloc(254 + 1);
		::GetWindowText(::GetDlgItem(hDlg, IDC_TYPES), temp.data, 254);

		if (temp.Len()) {
			confIni.Write("masks", counter.Sf("%d", i), temp.GetSafe());
			savedStrings.Sf("-|-%s-|-", temp.GetSafe());
		}
		else
			savedStrings = "";

		while((length = (INT)::SendMessage(::GetDlgItem(hDlg, IDC_TYPES), CB_GETLBTEXTLEN, (WPARAM)i, 0L)) != CB_ERR) {
			temp.Realloc(length + 1);
			::SendMessage(::GetDlgItem(hDlg, IDC_TYPES), CB_GETLBTEXT, (WPARAM)i++, (LPARAM)temp.GetSafe());

			if (!savedStrings.Find(matchString.Sf("-|-%s-|-", temp.GetSafe()), found)) {
				confIni.Write("masks", counter.Sf("%d", i), temp.GetSafe());
				savedStrings.Cat("-|-");
				savedStrings.Cat(temp.GetSafe());
				savedStrings.Cat("-|-");
			}
		}

		// where. Save folders: each one in a different entry
		confIni.Delete("where");
		i = 0;

		// First Read current selection
		temp.Realloc(254 + 1);
		::GetWindowText(::GetDlgItem(hDlg, IDC_WHERE), temp.data, 254);

		if (temp.Len()) {
			confIni.Write("where", counter.Sf("%d", i), temp.GetSafe());
			savedStrings.Sf("-|-%s-|-", temp.GetSafe());
		}
		else
			savedStrings = "";

		while((length = (INT)::SendMessage(::GetDlgItem(hDlg, IDC_WHERE), CB_GETLBTEXTLEN, (WPARAM)i, 0L)) != CB_ERR) {
			temp.Realloc(length + 1);
			::SendMessage(::GetDlgItem(hDlg, IDC_WHERE), CB_GETLBTEXT, (WPARAM)i++, (LPARAM)temp.GetSafe());

			if (!savedStrings.Find(matchString.Sf("-|-%s-|-", temp.GetSafe()), found)) {
				confIni.Write("where", counter.Sf("%d", i), temp.GetSafe());
				savedStrings.Cat("-|-");
				savedStrings.Cat(temp.GetSafe());
				savedStrings.Cat("-|-");
			}
		}
	}
	catch (...) {}
}

void SearchInFilesDock::LoadCombosStrings(HWND hDlg) {
	try {
		/////////////////////////////////////////////////////////////////
		// Load the combos with the content found in the inifile
		CUT2_INI	confIni(m_iniFilePath);
		CUT2_INI	confRead(m_iniFilePath);
		CUTL_BUFFER inputBuf;
		LPCSTR		entry;
		int			i = 0;

		HWND hComboWhat  = ::GetDlgItem(hDlg, IDC_WHAT);
		HWND hComboTypes = ::GetDlgItem(hDlg, IDC_TYPES);
		HWND hComboWhere = ::GetDlgItem(hDlg, IDC_WHERE);

		// Load what entries
		confIni.LoadEntries("what");

		while (!!(entry = confIni.GetEntry(i))) {
			CUTL_BUFFER itemTag = confRead.LoadStr("what", entry);

			::SendMessage(hComboWhat, CB_ADDSTRING, 0, (LPARAM)itemTag.GetSafe());
			i++;
		}
		if (i) ::SendMessage(hComboWhat, CB_SETCURSEL, 0, 0L);



		// Load 'masks' entries
		i = 0;
		confIni.LoadEntries("masks");

		while (!!(entry = confIni.GetEntry(i))) {
			CUTL_BUFFER itemTag = confRead.LoadStr("masks", entry);

			::SendMessage(hComboTypes, CB_ADDSTRING, 0, (LPARAM)itemTag.GetSafe());
			i++;
		}
		if (i) ::SendMessage(hComboTypes, CB_SETCURSEL, 0, 0L);

		// Load 'where' entries
		i = 0;
		confIni.LoadEntries("where");

		while (!!(entry = confIni.GetEntry(i))) {
			CUTL_BUFFER itemTag = confRead.LoadStr("where", entry);

			::SendMessage(hComboWhere, CB_ADDSTRING, 0, (LPARAM)itemTag.GetSafe());
			i++;
		}
		if (i) ::SendMessage(hComboWhere, CB_SETCURSEL, 0, 0L);
	}
	catch (...) {}
}

void SearchInFilesDock::SaveChecks(HWND hDlg) {
	try {
		CUT2_INI	confIni(m_iniFilePath);

		// Save checks
		confIni.Write(getSectionName(), "caseSensitive", 
			(::SendMessage(::GetDlgItem(hDlg, IDC_CHECK_CASE_SENSITIVE), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? "1" : "0");

		confIni.Write(getSectionName(), "wholeWord", 
			(::SendMessage(::GetDlgItem(hDlg, IDC_WHOLE_WORD), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? "1" : "0");

		confIni.Write(getSectionName(), "resultsInNewTab", 
			(::SendMessage(::GetDlgItem(hDlg, IDC_RESULTS_IN_NEW_TAB), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? "1" : "0");
	 
		confIni.Write(getSectionName(), "excludeExtensions", 
			(::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_EXTENSIONS), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? "1" : "0");
	}
	catch (...) {}
}

void SearchInFilesDock::LoadChecks(HWND hDlg) {
	try {
		CUT2_INI	confIni(m_iniFilePath);

		// Read checks previous state
		::SendMessage(::GetDlgItem(hDlg, IDC_CHECK_CASE_SENSITIVE), 
						BM_SETCHECK,        
						(WPARAM) confIni.LoadInt(getSectionName(), "caseSensitive", 0) ? BST_CHECKED : BST_UNCHECKED,
						0L);

		::SendMessage(::GetDlgItem(hDlg, IDC_WHOLE_WORD), 
						BM_SETCHECK,        
						(WPARAM) confIni.LoadInt(getSectionName(), "wholeWord", 0) ? BST_CHECKED : BST_UNCHECKED,
						0L);

		::SendMessage(::GetDlgItem(hDlg, IDC_RESULTS_IN_NEW_TAB), 
						BM_SETCHECK,        
						(WPARAM) confIni.LoadInt(getSectionName(), "resultsInNewTab", 0) ? BST_CHECKED : BST_UNCHECKED,
						0L);

		::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_EXTENSIONS), 
						BM_SETCHECK,        
						(WPARAM) confIni.LoadInt(getSectionName(), "excludeExtensions", 1) ? BST_CHECKED : BST_UNCHECKED,
						0L);
	}
	catch (...) {}
}

//////////////////////////////////////////////////////////////////////////////////////////////
// SearchInputDlg class
//////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK SearchInputDlg::SearchInFilesInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static SearchInFilesDock* ownerDlg;
	try {
		switch(message)
		{
			case WM_INITDIALOG:
				{
					RECT rc, rcDlg;
					POINT upperMiddle;

					// Asign owner
					ownerDlg = (SearchInFilesDock*)lParam;

					// Load Combos Strings
					ownerDlg->LoadCombosStrings(hDlg);

					// Load Checks state
					ownerDlg->LoadChecks(hDlg);
		
					// Ask Scintilla for the current selected text
					size_t selectionStart = ::SendMessage(ownerDlg->m_scintillaMainHandle, SCI_GETSELECTIONSTART, 0, 0L);
					size_t selectionEnd = ::SendMessage(ownerDlg->m_scintillaMainHandle, SCI_GETSELECTIONEND, 0, 0L);

					size_t strSize = ((selectionEnd > selectionStart) ? (selectionEnd - selectionStart) : (selectionStart - selectionEnd)) + 1;

					// Is there any selection at all?
					if (selectionStart != selectionEnd) {
						CUTL_BUFFER selectionText((int)strSize);

						::SendMessage(ownerDlg->m_scintillaMainHandle, SCI_GETSELTEXT, 0, (LPARAM)selectionText.data);

						if (selectionText.Len() > 100) {
							CUTL_BUFFER tempBuf;

							tempBuf.NCopy(selectionText.GetSafe(), 100);
							selectionText = tempBuf;
						}

						::SendMessage(::GetDlgItem(hDlg, IDC_WHAT), CB_INSERTSTRING, 0, (LPARAM)selectionText.GetSafe());
						::SendMessage(::GetDlgItem(hDlg, IDC_WHAT), CB_SETCURSEL, 0, 0L);
					}

					/////////////////////////////////////////////////////////////////
					// Place the dialog
					CUT2_INI confIni(ownerDlg->m_iniFilePath);

					int	x = confIni.LoadInt(ownerDlg->getSectionName(), "dlgInputLeft", -1);
					int	y = confIni.LoadInt(ownerDlg->getSectionName(), "dlgInputTop", -1);

					::GetWindowRect(hDlg, &rcDlg);
					::GetWindowRect(::GetDesktopWindow(), &rc);

					if ((x == -1 && y == -1) || (y > (rc.bottom - 100)) || (x > rc.right)) {
						upperMiddle.x = rc.left + ((rc.right - rc.left) / 2);
						upperMiddle.y = rc.top + ((rc.bottom - rc.top) / 4);

						x = upperMiddle.x - ((rcDlg.right - rcDlg.left) / 2);
						y = upperMiddle.y - ((rcDlg.bottom - rcDlg.top) / 2);
					}

					// Hide 24px on the lower part of the dialog
					int dlgHeight = (rcDlg.bottom - rcDlg.top) - 24;

					::SetWindowPos(hDlg, HWND_TOP, x, y, rcDlg.right - rcDlg.left, dlgHeight, SWP_SHOWWINDOW);
				}

			case WM_COMMAND:
				if(LOWORD(wParam) == IDCANCEL) {
					if (ownerDlg->m_bInProcess == true) {
						ownerDlg->m_bStopPressed = true;
						::EnableWindow(::GetDlgItem(hDlg, IDCANCEL), FALSE);
					}
					else
						::EndDialog(hDlg, 0);
					return TRUE;
				}
				
				if (LOWORD(wParam) == IDC_CHOOSE_FOLDER) {
					ownerDlg->chooseFolder(hDlg);
					return TRUE;
				}

				if (LOWORD(wParam) == IDC_MANAGE_EXTENSIONS) {
					DialogBoxParam(ownerDlg->getHinst(), 
									(LPCTSTR)IDD_EXCLUDE_EXTENSIONS, 
									hDlg, 
									(DLGPROC)SearchInputDlg::SearchInFilesExcludeDlgProc,
									(LPARAM)ownerDlg->m_iniFilePath);
				}

				if (LOWORD(wParam) == IDOK) {
					// Validaciones primero
					CUTL_BUFFER what(255), types(255), where(255);

					::GetDlgItemText(hDlg, IDC_WHAT, what, 254);
					::GetDlgItemText(hDlg, IDC_TYPES, types, 254);
					::GetDlgItemText(hDlg, IDC_WHERE, where, 254);

					if (!what.Len()) {
						::MessageBox(ownerDlg->getHParent(), "You must supply a pattern string to search ...", "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_WHAT));
						return FALSE;
					}
					if (!types.Len()) {
						::MessageBox(ownerDlg->getHParent(), "You must supply a type file mask ...", "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_TYPES));
						return FALSE;
					}
					if (!where.Len()) {
						::MessageBox(ownerDlg->getHParent(), "You must supply a root folder to search ...", "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_WHERE));
						return FALSE;
					}

					// Verify the given folder exists
					CUTL_PATH wherePath(where.GetSafe());

					if (!wherePath.DirectoryExists()) {
						CUTL_BUFFER msgBuf;

						::MessageBox(ownerDlg->getHParent(), msgBuf.Sf("The suplied folder '%s', does not exists", where.GetSafe()), "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_WHERE));
						return FALSE;
					}

					ownerDlg->callSearchInFiles(hDlg, what, types, where);
				}
				break;

			case WM_DESTROY:
				{
					// Save Combos
					ownerDlg->SaveCombosStrings(hDlg);

					// Save Checks
					ownerDlg->SaveChecks(hDlg);

					// Let's save save the dialog coordinates
					RECT		dlgRect;
					CUT2_INI	confIni(ownerDlg->m_iniFilePath);

					::GetWindowRect(hDlg, &dlgRect);
					confIni.Write(ownerDlg->getSectionName(), "dlgInputLeft", dlgRect.left);
					confIni.Write(ownerDlg->getSectionName(), "dlgInputTop", dlgRect.top);
				}
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error en SearchInputDlg::SearchInFilesInputDlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

BOOL CALLBACK SearchInputDlg::SearchInFilesExcludeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{
	static char m_iniFilePath[MAX_PATH];

	try {
		switch(message)
		{
			case WM_INITDIALOG:
				{
					UTL_strcpy(m_iniFilePath, (LPSTR)lParam);

					// Fill Exclued extensions list
					CUT2_INI confIni(m_iniFilePath);
					CUTL_BUFFER excludeExtensionsList, tempBuf;

					excludeExtensionsList = confIni.LoadStr("Search in Files Extension", "excludeExtensionsList", "");

					if (excludeExtensionsList.Len()) {
						excludeExtensionsList.RepCar(';', ',');
						tempBuf.Sf("#%s;", excludeExtensionsList.GetSafe());

						CUTL_PARSE excludeParse(tempBuf.GetSafe(), NULL, '#');

						excludeParse.NextToken();

						for (UINT i = 1; i <= excludeParse.NumArgs(); i++) 
							::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_ADDSTRING, 0, (LPARAM)excludeParse.StrArg(i));
					}

					// Place the dialog
					RECT rc, rcDlg;
					POINT upperMiddle;

					::GetWindowRect(::GetDesktopWindow(), &rc);
					::GetWindowRect(hDlg, &rcDlg);

					upperMiddle.x = rc.left + ((rc.right - rc.left) / 2);
					upperMiddle.y = rc.top + ((rc.bottom - rc.top) / 4);

					int x = upperMiddle.x - ((rcDlg.right - rcDlg.left) / 2);
					int y = upperMiddle.y - ((rcDlg.bottom - rcDlg.top) / 2);

					::SetWindowPos(hDlg, HWND_TOP, x, y, rcDlg.right - rcDlg.left, rcDlg.bottom - rcDlg.top, SWP_SHOWWINDOW);
				}
				break;

			case WM_COMMAND:
				{
					if (LOWORD(wParam) == IDC_EXCLUDE_EXT) {
						// Read the new extension
						CUTL_BUFFER newExtension(256);

						::GetWindowText(::GetDlgItem(hDlg, IDC_EXTENSION), (LPSTR)newExtension.GetSafe(), 255);

						// Is it already on the list?
						if (LB_ERR != ::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_FINDSTRINGEXACT, -1, (LPARAM)newExtension.GetSafe())) {
							::MessageBox(hDlg, "This extension is alredy on the list", "Search in Files", MB_OK);
							::SendMessage(::GetDlgItem(hDlg, IDC_EXTENSION), EM_SETSEL, 0, -1);
						}
						else {
							::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_ADDSTRING, 0, (LPARAM)newExtension.GetSafe());
							::SetWindowText(::GetDlgItem(hDlg, IDC_EXTENSION), "");

						}
						::SetFocus(::GetDlgItem(hDlg, IDC_EXTENSION));
						return FALSE;
					}

					if (LOWORD(wParam) == IDC_EXCLUDE_LIST) {
						if (HIWORD(wParam) == LBN_DBLCLK) {


							int selIndex = (int)::SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0L);

							if (selIndex != LB_ERR) 
								::SendMessage((HWND)lParam, LB_DELETESTRING, selIndex, 0l);
						}
						return FALSE;
					}

					if (LOWORD(wParam) == IDOK) {
						// Save the extension files to exclude
						CUTL_BUFFER extensionsToExclude;
						CUTL_BUFFER temp;

						int i = 0, length;

						while(LB_ERR != (length = (int)::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_GETTEXTLEN, (WPARAM)i, 0L))) {
							temp.Realloc(length + 1);

							::SendMessage(::GetDlgItem(hDlg, IDC_EXCLUDE_LIST), LB_GETTEXT, (WPARAM)i++, (LPARAM)temp.GetSafe());

							if (extensionsToExclude.Len()) extensionsToExclude += ";";
							
							extensionsToExclude += temp.GetSafe();
						}

						if (extensionsToExclude.Len()) {
							CUT2_INI	confIni(m_iniFilePath);

							confIni.Write("Search in Files Extension", "excludeExtensionsList", extensionsToExclude.GetSafe());
						}
						::EndDialog(hDlg, IDOK);
					}

					if (LOWORD(wParam) == IDCANCEL) {
						::EndDialog(hDlg, IDCANCEL);
					}
				}
				break;

			default:
				break;
		}
	}
	catch (...) {
		systemMessageEx("Error en SearchInputDlg::SearchInFilesInputDlgProc", __FILE__, __LINE__);
	}
	return FALSE;
}

