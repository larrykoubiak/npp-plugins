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
#include "SearchResultsListCtrl.h"
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
				m_searchResultsListCtrl.SubclassWindow(::GetDlgItem(_hSelf, IDC_RESULTSLIST), this);

				// Create a font using the system message font
				NONCLIENTMETRICS ncm;

				ncm.cbSize = sizeof(NONCLIENTMETRICS);
				if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
					m_font.CreateFontIndirect(&(ncm.lfMessageFont));
				else 
					m_font.CreateFontA(-11,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,"Tahoma");

				m_searchResultsListCtrl.SetFont(m_font);
				return TRUE;
			}

			case WM_DESTROY:
			{
				m_searchResultsListCtrl.DestroyWindow();
				return TRUE;
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
				CRect rc;

				getClientRect(rc);

				rc.DeflateRect(2, 2);
				m_searchResultsListCtrl.MoveWindow(&rc);
			}
			break; 

			case WM_NOTIFY:
			{
				NMHDR* pNMHDR = (LPNMHDR)lParam;

				if (pNMHDR->hwndFrom == m_searchResultsListCtrl.m_hWnd) {
					switch (pNMHDR->code) {
						case TVN_DELETEITEM:
							m_searchResultsListCtrl.onDeleteItem(pNMHDR);
							break;

						default:
							break;
					}
				}
			}
			break;


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
	CUTL_BUFFER temp;

	// Read checks
	bool bWholeWord		  = (::SendMessage(::GetDlgItem(hDlg, IDC_WHOLE_WORD), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? true : false;
	bool bResultsInNewTab = (::SendMessage(::GetDlgItem(hDlg, IDC_RESULTS_IN_NEW_TAB), BM_GETCHECK, 0, 0L) == BST_CHECKED) ? true : false;

	temp.Sf("%s   %s   %s", what.GetSafe(), types.GetSafe(), where.GetSafe());

	if (!bResultsInNewTab) {
		//_ctrlTab.renameTab(_ctrlTab.getCurrentTab(), temp.GetSafe());
	}
	else {
		/*
		¿Hay que abrir la segunda ventana?
		*/
	}

	// Save the length of the current search
	m_iCurrSearchLength = bWholeWord ? what.strlen() + 2 : what.strlen();

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
		if (m_searchResultsListCtrl.GetCount() < 2) return;

		HTREEITEM itemSel		= m_searchResultsListCtrl.GetSelectedItem();
		HTREEITEM itemParent	= m_searchResultsListCtrl.GetParentItem(itemSel);
		HTREEITEM itemNext;

		CCustomItemInfo* pCii = (CCustomItemInfo*)m_searchResultsListCtrl.GetItemData(itemSel);

		if (pCii == NULL) {
			if (itemSel == m_searchResultsListCtrl.GetMessageItem())
				itemParent = m_searchResultsListCtrl.GetNextSiblingItem(itemSel);

			itemNext = m_searchResultsListCtrl.GetChildItem(itemParent);
		}
		else {
			// Let's find the next leave
			itemNext = m_searchResultsListCtrl.GetNextSiblingItem(itemSel);

			if (itemNext == NULL) {
				itemParent = m_searchResultsListCtrl.GetNextSiblingItem(itemParent);

				if (itemParent == NULL) {
					itemParent = m_searchResultsListCtrl.GetMessageItem();
					itemParent = m_searchResultsListCtrl.GetNextSiblingItem(itemParent);
					itemNext   = m_searchResultsListCtrl.GetChildItem(itemParent);
				}
				else
					itemNext   = m_searchResultsListCtrl.GetChildItem(itemParent);
			}
		}

		m_searchResultsListCtrl.SetItemState(itemSel, ~(TVGN_CARET | TVIS_SELECTED), TVGN_CARET | TVIS_SELECTED);
		openCurrSelection(itemNext);
	}
	catch (...) {
		systemMessageEx("Error at SearchInFilesDock::moveToNextHit.", __FILE__, __LINE__);
	}
}

void SearchInFilesDock::openCurrSelection(HTREEITEM selItem) {
	try {
		CUTL_BUFFER fileToOpen;
		CUTL_BUFFER filePath(MAX_PATH + 1), fileName(MAX_PATH + 1), lineNumber(MAX_PATH + 1), col(MAX_PATH + 1);

		CCustomItemInfo* pCii = (CCustomItemInfo*)m_searchResultsListCtrl.GetItemData(selItem);

		if (pCii == NULL) return;

		m_searchResultsListCtrl.EnsureVisible(selItem);
		m_searchResultsListCtrl.SelectItem(selItem);

		::SendMessage(m_nppHandle, WM_DOOPEN, 0, (LPARAM)(LPSTR)pCii->m_fullPath.GetSafe());

		int startPos = (int)::SendMessage(m_scintillaMainHandle, SCI_POSITIONFROMLINE, pCii->m_line - 1, 0L);

		startPos += pCii->m_column;

		int endPos = startPos + m_iCurrSearchLength;
		::SendMessage(m_scintillaMainHandle, SCI_SETSEL, startPos, endPos);
	}
	catch(...) {
		systemMessageEx("Error at searchResultListCtrl::openCurrSelection", __FILE__, __LINE__);
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

		if (temp.strlen()) {
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

		if (temp.strlen()) {
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

		if (temp.strlen()) {
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

						if (selectionText.strlen() > 100) {
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

					if (!what.strlen()) {
						::MessageBox(ownerDlg->getHParent(), "You must supply a pattern string to search ...", "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_WHAT));
						return FALSE;
					}
					if (!types.strlen()) {
						::MessageBox(ownerDlg->getHParent(), "You must supply a type file mask ...", "Search in Files", MB_OK|MB_ICONASTERISK);
						::SetFocus(::GetDlgItem(hDlg, IDC_TYPES));
						return FALSE;
					}
					if (!where.strlen()) {
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

					if (excludeExtensionsList.strlen()) {
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

							if (extensionsToExclude.strlen()) extensionsToExclude += ";";
							
							extensionsToExclude += temp.GetSafe();
						}

						if (extensionsToExclude.strlen()) {
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

