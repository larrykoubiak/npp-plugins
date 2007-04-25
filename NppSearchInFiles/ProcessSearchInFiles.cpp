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


#include "stdafx.h"

#include "resource.h"

#include "dockingFeature/staticDialog.h"
#include "SearchResultsListCtrl.h"
#include "SearchInFilesDock.h"

#include "ProcessSearchInFiles.h"

void CProcessSearchInFiles::doSearch() {
	if (m_searchInputDlgHnd == NULL || m_searchDock == NULL || m_mainDock == NULL) {
		::MessageBox(NULL, "CProcessSearchInFiles::doSearch: object not inicialized.", "Search in Files", MB_OK);
		return;
	}

	// Let's start
	// Change IDCANCEL caption
	::SetFocus(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL));
	::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), "&Stop");
	// Disable OK button
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDOK), FALSE);
	// Disable other controls
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_TYPES), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHERE), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_CHOOSE_FOLDER), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_CHECK_CASE_SENSITIVE), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHOLE_WORD), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_RESULTS_IN_NEW_TAB), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_EXCLUDE_EXTENSIONS), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_INCLUDE_SUBFOLDERS), FALSE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_MANAGE_EXTENSIONS), FALSE);

	m_mainDock->m_bInProcess = true;
	m_mainDock->m_bStopPressed = false;
	ResizeInputDgl();

	m_currRootItem	= NULL;
	m_currHitFile	= "";

	bool searchResult = false;

	// Empty previous searches
	m_pSearchDockList->DeleteAllItems();

	// Clean search variables
	m_pSearchDockList->SetFirstItem(NULL);
	m_totalHits = 0;
	m_totalFolders = 0;
	m_totalFiles = 0;
	m_percentageProgress = "0%";
	::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDC_STATIC_PROGRESS), "");
	::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), PBM_SETRANGE, 0L, (LPARAM) MAKELPARAM (0, 100));
	::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), PBM_SETPOS, (WPARAM)0, 0L);

	try {
		// Read search
		CUTL_BUFFER staticStatusBuf, what(256), types(256), where(256);

		::GetDlgItemText(m_searchInputDlgHnd, IDC_WHAT, what, 255);
		::GetDlgItemText(m_searchInputDlgHnd, IDC_TYPES, types, 255);
		::GetDlgItemText(m_searchInputDlgHnd, IDC_WHERE, where, 255);

		m_bCaseSens = ::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_CHECK_CASE_SENSITIVE),
						BM_GETCHECK,			// message to send
						(WPARAM)0,				// not used; must be zero
						(LPARAM)0) == BST_CHECKED ? true : false;

		m_bWholeWord = ::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_WHOLE_WORD),
						BM_GETCHECK,			// message to send
						(WPARAM)0,				// not used; must be zero
						(LPARAM)0) == BST_CHECKED ? true : false;

		m_bIncludeSubfolders = ::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_INCLUDE_SUBFOLDERS),
						BM_GETCHECK,			// message to send
						(WPARAM)0,				// not used; must be zero
						(LPARAM)0) == BST_CHECKED ? true : false;

		m_bExcludeExtensions = ::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_EXCLUDE_EXTENSIONS),
						BM_GETCHECK,			// message to send
						(WPARAM)0,				// not used; must be zero
						(LPARAM)0) == BST_CHECKED ? true : false;


		// Read excluded extensions (ONLY WHEN SEARCHING *.*!)
		CUT2_INI	confIni(m_mainDock->m_iniFilePath);
		
		if (m_bExcludeExtensions && !UTL_strcmp(types.GetSafe(), "*.*")) {
			m_excludeExtensionsList = confIni.LoadStr(m_mainDock->getSectionName(), "excludeExtensionsList", "");

			if (m_excludeExtensionsList.strlen()) {
				m_excludeExtensionsList.RepCar(';', ',');
		
				CUTL_BUFFER temp;

				temp.Sf("#%s;", m_excludeExtensionsList.GetSafe());
				m_excludeExtensionsList = temp.GetSafe();
			}
		}
		else
			m_bExcludeExtensions = false;

		/////////////////////////////////////////////////////////////////
		// Start
		CUTL_BUFFER strFolder(MAX_PATH + 1), tempBuf;

		::GetDlgItemText(m_searchInputDlgHnd, IDC_WHERE, strFolder, MAX_PATH);
		m_foldersArray.Realloc(0);

		// Search for subfolders
		if (searchResult = SearchFolders(strFolder.GetSafe())) 
			searchResult = SearchInFolders(); // Do the search in files process

		staticStatusBuf.Sf("(%d hits)  '%s' - '%s' - '%s'", 
							m_totalHits,
							what.GetSafe(),
							types.GetSafe(),
							where.GetSafe());

		m_searchDock->UpdateWindowTitle(staticStatusBuf.GetSafe());
	}
	catch (...) {
		systemMessageEx("Error at SearchInFilesDock::callSearchInFiles", __FILE__, __LINE__);
	}

	m_mainDock->m_bInProcess = false;
	m_mainDock->m_bStopPressed = false;
	ResizeInputDgl();
	// Enable OK button
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDOK), TRUE);
	// Enable the cancel button (maybe it was disabled when cancelling)
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), TRUE);
	// Change IDCANCEL caption
	::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), "&Close");
	// Enable other controls
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_TYPES), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHERE), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_CHOOSE_FOLDER), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_CHECK_CASE_SENSITIVE), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_WHOLE_WORD), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_RESULTS_IN_NEW_TAB), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_EXCLUDE_EXTENSIONS), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_INCLUDE_SUBFOLDERS), TRUE);
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_MANAGE_EXTENSIONS), TRUE);

	// Place the focus on the first control
	::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT), EM_SETSEL, 0, -1);
	::SetFocus(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT));
}

bool CProcessSearchInFiles::checkCancelButton() {
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if(!::IsDialogMessage(m_searchInputDlgHnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);  
		}
	}

	if (m_mainDock->m_bStopPressed) {
		::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), TRUE);
		return true;
	}

	// Enable Cancel button (if it was pressed)
	if(m_mainDock->m_bStopPressed) ::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), TRUE);
	m_mainDock->m_bStopPressed = false;
	::SetFocus(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL));
	return false;
}

void CProcessSearchInFiles::ResizeInputDgl() {
	RECT	wndRect;
	POINT	coordinates;

	::GetWindowRect(m_searchInputDlgHnd, &wndRect);

	coordinates.x = wndRect.left;
	coordinates.y = wndRect.top;

	if (m_mainDock->m_bInProcess) {
		::ShowWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), SW_SHOW);
		wndRect.bottom += 24;
	}
	else {
		::ShowWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), SW_HIDE);
		::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDC_STATIC_PROGRESS), "");
		wndRect.bottom -= 24;
	}

	::SetWindowPos(m_searchInputDlgHnd, HWND_TOP, coordinates.x, coordinates.y, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, SWP_SHOWWINDOW);
}


bool CProcessSearchInFiles::SearchFolders(LPCSTR folder) {
	if (checkCancelButton()) return false;

	CUTL_BUFFER strFolder(folder);

	strFolder.Lower();   
   
	// We take this folder out of the search (05.10.07)
	//UINT pos;
	//if (strFolder.Find("jose\\wwwroot\\fotos", pos)) return true;

	if (strFolder[strFolder.strlen() - 1] != '\\') strFolder += "\\";

	CUTL_PATH   tempPath(strFolder.GetSafe());
	CUTL_BUFFER nextFolder, msg;

	if (m_foldersArray.strlen() == 0) 
		m_foldersArray = "#";
	else
		m_foldersArray += ",";

	m_foldersArray += folder;

	if (!m_bIncludeSubfolders) return true; // If subfolders are not to be included, we're done

	// Tell what we're doing
	m_searchDock->UpdateWindowTitle(msg.Sf("Added folder %s", folder));

	tempPath.SetNameExtension("*.*");
	// Search for subfolders
	if (tempPath.FindFirst(_A_SUBDIR)) {
		if (checkCancelButton()) return false;

		if (strFolder[0] == '\\')
			nextFolder = (LPCSTR)tempPath;
		else
			tempPath.GetDriveDirectory(nextFolder);

		if (!SearchFolders((LPSTR)nextFolder)) return false;

		while(tempPath.FindNext()) {
			if (strFolder[0] == '\\')
				nextFolder = (LPCSTR)tempPath;
			else
				tempPath.GetDriveDirectory(nextFolder);

			if (!SearchFolders((LPSTR)nextFolder)) return false;
		}
	}   
	return true;
}

// We return false for and error or because the user asked
bool CProcessSearchInFiles::FindInfile(LPCSTR file) {
	FILE *fp = NULL;

	try {
		CUTL_BUFFER searchPattern(MAX_PATH);
		CUTL_PATH   iterator(file); 
		CUTL_BUFFER lineToShow, extension, tempBuf;
		int         line; 

		// We skip some extensions (if they told us so)
		iterator.GetExtension(extension);
		extension.Lower();

		// Let's check if we should exclude this file from search (ONLY WHEN SEARCHING *.*!)
		if (m_bExcludeExtensions && m_excludeExtensionsList.strlen()) {
			CUTL_PARSE excludeParse(m_excludeExtensionsList.GetSafe(), NULL, '#');

			excludeParse.NextToken();

			for (UINT i = 1; i <= excludeParse.NumArgs(); i++) {
				tempBuf = excludeParse.StrArg(i);

				if (!UTL_strcmp(extension.GetSafe(), tempBuf.Lower().GetSafe()))
					return true;
			}
		}

		line = 1;

		::GetDlgItemText(m_searchInputDlgHnd, IDC_WHAT, searchPattern, MAX_PATH);

		CUTL_BUFFER		StringFile, tempStringFile, StringLine, err;
		UINT			hitPos, startPos = 0, endPos = 0;

		// Let's read the file
		int fileSize = FileSize(file);

		if (!fileSize) return true;

		fp = fopen(file, "r");
	    
		if (!fp) return true;
		StringFile.Realloc(fileSize + 1);
		int lenFile = int(fread(StringFile, 1, fileSize, fp));
		fclose(fp);

		StringFile[lenFile] = '\0';

		tempStringFile = StringFile;

		// Let's take care of the checks
		if (!m_bCaseSens) {
			tempStringFile.Upper();
			searchPattern.Upper();
		}

		//if (m_bWholeWord) {
		//	tempBuf = searchPattern;
		//	searchPattern.Sf(" %s ", tempBuf.GetSafe());
		//}

		while (tempStringFile.Find("\n", hitPos, endPos)) {
			if (checkCancelButton()) return false;

			if (hitPos - endPos > 128) {
				lineToShow.NCopy(&StringFile[endPos], 128);
				lineToShow += "...";
			}
			else
				lineToShow.NCopy(&StringFile[endPos], hitPos - endPos);
			if (!FindInLine(tempBuf.NCopy(&tempStringFile[endPos], hitPos - endPos), lineToShow.GetSafe(), searchPattern, iterator, line++))
				return false;

			endPos = hitPos + 1;
		}
      
		// Tratamos el resto del buffer.
		if (endPos < tempStringFile.strlen()) {
			if (tempStringFile.strlen() - endPos > 128) {
				lineToShow.NCopy(&StringFile[endPos], 128);
				lineToShow += "...";
			}
			else
				lineToShow.NCopy(&StringFile[endPos], tempStringFile.strlen() - endPos);
			if (!FindInLine(&tempStringFile[endPos], lineToShow.GetSafe(), searchPattern, iterator, line))
				return false;
		}
	}
	catch (...) {
		// We close the (maybe) opened file
		if (fp != NULL) fclose(fp);

		CUTL_BUFFER err;
		systemMessageEx(err.Sf("Error at CProcessSearchInFiles::FindInfile: '%s'.", file), __FILE__, __LINE__);
		return false;
	}
	return true;
}

int CProcessSearchInFiles::FileSize(const char * szFileName) { 
  struct stat fileStat; 
  int err = stat(szFileName, &fileStat); 
  if (0 != err) return 0; 
  return fileStat.st_size; 
}

BOOL CProcessSearchInFiles::DoFind(LPCSTR strLine, LPCSTR searchPattern, UINT& hitPos, UINT endPosLine, BOOL& wholeWordSuccess) {
	CUTL_BUFFER		bufStringLine(strLine);
	BOOL			success = bufStringLine.Find(searchPattern, hitPos, endPosLine);
	CUTL_BUFFER		set("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

	// If the search is wholeWord we have to make more questions before giving a hit
	if (success && m_bWholeWord) {
		UCHAR	beforeChar, afterChar;
		UINT	found;

		wholeWordSuccess = FALSE;

		if (hitPos == 0) {
			// Only look at the character after the pattern
			if (UTL_strlen(strLine) == UTL_strlen(searchPattern))
				wholeWordSuccess = TRUE;
			else {
				afterChar = bufStringLine[hitPos + UTL_strlen(searchPattern)];

				wholeWordSuccess = afterChar < 128 && !set.Find(afterChar, found);
			}
		}
		else {
			if (UTL_strlen(strLine) > (int)(hitPos + UTL_strlen(searchPattern))) {
				// Look at the characteres before and after the pattern
				beforeChar = bufStringLine[hitPos - 1];
				afterChar = bufStringLine[hitPos + UTL_strlen(searchPattern)];

				wholeWordSuccess = afterChar < 128 && !set.Find(beforeChar, found) && 
								   beforeChar < 128 && !set.Find(afterChar, found);
			}
			else {
				// Look only at the character before the pattern
				beforeChar = bufStringLine[hitPos - 1];

				wholeWordSuccess = beforeChar < 128 && !set.Find(beforeChar, found);
			}
		}
	}
	return success;
}

bool CProcessSearchInFiles::FindInLine(LPCSTR strLine, LPCSTR lineToShow, LPCSTR searchPattern, CUTL_PATH iterator, int line) {
	try {
		UINT			hitPos, found, endPosLine = 0;
		CUTL_BUFFER		bufLine, nameExtension, driveDirectory, temp;
		BOOL			wholeWordSuccess;
		TV_INSERTSTRUCT tvis;

		//while (StringLine.Find(searchPattern, hitPos, endPosLine)) {
		while(DoFind(strLine, searchPattern, hitPos, endPosLine, wholeWordSuccess)) {
			endPosLine = hitPos + 1;
			if (m_bWholeWord && !wholeWordSuccess) continue;

			bufLine = lineToShow;
			bufLine.RepCar('\t', ' ');
			bufLine.RepCar('\n', ' ');
			bufLine.RepCar('\r', ' ');

			iterator.GetNameExtension(nameExtension);
			if (*(LPCSTR)iterator == '\\') {
				driveDirectory = (LPCSTR)iterator;
				if (driveDirectory.ReverseFind('\\', found)) 
					driveDirectory[found] = '\0';
			}
			else
				iterator.GetDriveDirectory(driveDirectory);

			if (UTL_strcmp((LPCSTR)iterator, (LPCSTR)m_currHitFile)) {
				m_currFileHits = 0;

				ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));

				tvis.hParent				= TVI_ROOT;
				tvis.hInsertAfter			= TVI_LAST;
				tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
				tvis.item.pszText			= (LPSTR)temp.Sf("Hits %5d  |  %s", m_currFileHits, (LPCSTR)iterator);
				tvis.item.iImage			= GetIconIndex((LPCSTR)iterator);
				tvis.item.iSelectedImage	= GetSelIconIndex((LPCSTR)iterator);
				tvis.item.cChildren			= true;
				tvis.item.lParam			= NULL;

				m_currRootItem = m_pSearchDockList->InsertItem(&tvis);

				if (m_pSearchDockList->GetFirstItem() == NULL) m_pSearchDockList->SetFirstItem(m_currRootItem);

				m_currHitFile	= iterator;
				m_currFileIcon  = tvis.item.iImage;
				m_pSearchDockList->SetItemState(m_currRootItem, TVIS_EXPANDED, TVIS_EXPANDED);
			}

			ZeroMemory(&tvis, sizeof(TV_INSERTSTRUCT));

			tvis.hParent				= m_currRootItem;
			tvis.hInsertAfter			= TVI_LAST;
			tvis.item.mask				= TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM;
			tvis.item.pszText			= (LPSTR)temp.Sf("%5d: %s", line, bufLine.GetSafe());
			tvis.item.iImage			= m_currFileIcon;
			tvis.item.iSelectedImage	= m_currFileIcon;
			tvis.item.cChildren			= false;

			CCustomItemInfo* pCii = new CCustomItemInfo(line, hitPos, (LPCSTR)iterator);

			tvis.item.lParam			= (LPARAM)pCii;
			m_pSearchDockList->InsertItem(&tvis);

			m_totalHits++;
			m_currFileHits++;

			m_pSearchDockList->SetItemText(m_currRootItem, temp.Sf("(%d) %s", m_currFileHits, (LPCSTR)iterator));

			if (checkCancelButton()) return false;
		}
	}
	catch (...) {
		systemMessageEx("Error at CProcessSearchInFiles::FindInLine", __FILE__, __LINE__);
	}
	return true;
}

bool CProcessSearchInFiles::SearchInFolders() {
	UINT        i, j, filesCounter = 0;
	CUTL_BUFFER fileMaskEdit(255), bufFileMask, statusText;

	// We use the CUTL_PARSE class
	if (m_foldersArray[m_foldersArray.strlen() - 1] == ',')
		m_foldersArray[m_foldersArray.strlen() - 1] = ';';
	else
		m_foldersArray += ";";

	CUTL_PARSE	foldersParse(m_foldersArray, NULL, '#');

	::GetDlgItemText(m_searchInputDlgHnd, IDC_TYPES, fileMaskEdit, 254);

	foldersParse.NextToken();                                                
	m_totalFolders = foldersParse.NumArgs();

	::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), PBM_SETRANGE, 0L, (LPARAM) MAKELPARAM (0, foldersParse.NumArgs()));
	::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), PBM_SETPOS, (WPARAM)0, 0L);

	try {
		for (i = 1; i <= foldersParse.NumArgs(); i++) {
			::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_PROGRESS), PBM_SETPOS, (WPARAM)i, 0L);

			// Since progress is based on folders searched, we may have reached 100.00% but still have way to go.
			// To avoid the effect of having a 100.00% progress and still files to look at, we don't let 
			// 100.00% show on screen.
			m_percentageProgress.Sf("%.2lf%%", ((float)(100*i) / (float)foldersParse.NumArgs()));
			::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDC_STATIC_PROGRESS), m_percentageProgress == "100.00%" ? "99.00%" : m_percentageProgress.GetSafe());

			if (checkCancelButton()) return false;

			CUTL_PATH iterator;

			iterator.SetDriveDirectory(foldersParse.StrArg(i));
			if (!fileMaskEdit.strlen()) fileMaskEdit = "*.*";
			fileMaskEdit.RepCar(';', ',');
			bufFileMask.Sf("#%s;", fileMaskEdit.GetSafe());

			CUTL_PARSE fileMasksParse(bufFileMask.GetSafe(), NULL, '#');
      
			fileMasksParse.NextToken();                                                
			for (j = 1; j <= fileMasksParse.NumArgs(); j++) {
				iterator.SetNameExtension(fileMasksParse.StrArg(j));
				if (iterator.FindFirst(_A_NORMAL | _A_ARCH | _A_HIDDEN | _A_SYSTEM | _A_RDONLY)) {
					do { 
						statusText.Sf("Hits %5d  |  Folders %5d / %5d  |  Files %5d  |  %s", 
							m_totalHits, i, foldersParse.NumArgs(), ++m_totalFiles, (LPCSTR)iterator);

						m_searchDock->UpdateWindowTitle(statusText.GetSafe());

						if (checkCancelButton()) return false;
						if (!FindInfile((LPCSTR)iterator)) return false;
						if (checkCancelButton()) return false;
					} while (iterator.FindNext());
				}
				if (checkCancelButton()) return false;
			}
		} 
	}
	catch (...) {
		systemMessageEx("Error at CProcessSearchInFiles::SearchInFolders", __FILE__, __LINE__);
	}
	return true;
}

int CProcessSearchInFiles::GetIconIndex(const CUTL_BUFFER sFilename) {
	// Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;

	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	if(SHGetFileInfo(sFilename.GetSafe(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON) == 0)
		return -1;
	return sfi.iIcon;
}

int CProcessSearchInFiles::GetSelIconIndex(const CUTL_BUFFER sFilename) {
	// Retreive the icon index for a specified file/folder
	SHFILEINFO sfi;

	ZeroMemory(&sfi, sizeof(SHFILEINFO));
	if(SHGetFileInfo(sFilename.GetSafe(), 0, &sfi, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_OPENICON | SHGFI_SMALLICON ) == 0)
		return -1;
	return sfi.iIcon;
}

