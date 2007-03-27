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
	if (m_searchInputDlgHnd == NULL || m_searchDock == NULL) {
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
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_MANAGE_EXTENSIONS), FALSE);

	m_searchDock->m_bInProcess = true;
	m_searchDock->m_bStopPressed = false;
	ResizeInputDgl();

	bool searchResult = false;

	// Empty previous searches
	ListView_DeleteAllItems(m_searchDockListHWND);
	// Ask the list Control to adjust columns to its titles
	doFixedColumnsResize();
	// Clean search variables
	m_totalHits = 0;
	m_totalFolders = 0;
	m_totalFiles = 0;
	m_percentageProgress = "0%";
	::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDC_STATIC_PROGRESS), "");

	try {
		// Read search
		CUTL_BUFFER staticStatusBuf, what(256), types(256), where(256), endStatus;

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

		// Read excluded extensions (ONLY WHEN SEARCHING *.*!)
		CUT2_INI	confIni(m_searchDock->m_iniFilePath);
		
		m_bExcludeExtensions = confIni.LoadInt(m_searchDock->getSectionName(), "excludeExtensions", 0) ? true : false;

		if (m_bExcludeExtensions && !UTL_strcmp(types.GetSafe(), "*.*")) {
			m_excludeExtensionsList = confIni.LoadStr(m_searchDock->getSectionName(), "excludeExtensionsList", "");

			if (m_excludeExtensionsList.Len()) {
				m_excludeExtensionsList.RepCar(';', ',');
		
				CUTL_BUFFER temp;

				temp.Sf("#%s;", m_excludeExtensionsList.GetSafe());
				m_excludeExtensionsList = temp.GetSafe();
			}
		}
		else
			m_bExcludeExtensions = false;

		/////////////////////////////////////////////////////////////////
		// Start timming
		::SetWindowText(m_searchDockStaticHWND, "");
		float startTickCount = (float)::GetTickCount();

		CUTL_BUFFER strFolder(MAX_PATH + 1), tempBuf;

		::GetDlgItemText(m_searchInputDlgHnd, IDC_WHERE, strFolder, MAX_PATH);
		m_foldersArray.Realloc(0);

		// Search for subfolders
		if (!SearchFolders(strFolder.GetSafe())) goto END_PROCESS;
		// Do the search in files process
		searchResult = SearchInFolders();

		float endTickCount = (float)::GetTickCount();
		float eleapsedTime = ((endTickCount - startTickCount) / 1000);

		endStatus = searchResult ? "" : tempBuf.Sf("       (Search stopped at %s)", m_percentageProgress.GetSafe());

		staticStatusBuf.Sf("Hits %5d  |  Folders %5d  |  Files %5d  |  Search '%s' '%s' '%s'       in %0.3lf seconds%s     (Use F4 to navigate the results)", 
							m_totalHits, 
							m_totalFolders, 
							m_totalFiles, 
							what.GetSafe(),
							types.GetSafe(),
							where.GetSafe(),
							eleapsedTime,
							endStatus.GetSafe());

		::SetWindowText(m_searchDockStaticHWND, staticStatusBuf.GetSafe());
	}
	catch (...) {
		systemMessageEx("Error at SearchInFilesDock::callSearchInFiles", __FILE__, __LINE__);
	}

END_PROCESS:
	m_searchDock->m_bInProcess = false;
	m_searchDock->m_bStopPressed = false;
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
	::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDC_MANAGE_EXTENSIONS), TRUE);

	if (searchResult) {
		m_searchDock->m_bInProcess = false; // We are leaving
		::EndDialog(m_searchInputDlgHnd, IDCANCEL);
	}
	else {
		if (m_totalHits) {
			// Ask the list Control to adjust columns
			for (int i = 0; i < 6; i++)
				ListView_SetColumnWidth(m_searchDockListHWND, i, LVSCW_AUTOSIZE);		
		}
		else 
			doFixedColumnsResize();

		// Place the focus on the first control
		::SendMessage(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT), EM_SETSEL, 0, -1);
		::SetFocus(::GetDlgItem(m_searchInputDlgHnd, IDC_WHAT));
	}
}

// Ask the list Control to adjust columns to its titles
void CProcessSearchInFiles::doFixedColumnsResize() {
	ListView_SetColumnWidth(m_searchDockListHWND, 0, 190);		
	ListView_SetColumnWidth(m_searchDockListHWND, 1, 220);		
	ListView_SetColumnWidth(m_searchDockListHWND, 2, 160);		
	ListView_SetColumnWidth(m_searchDockListHWND, 3, 80);		
	ListView_SetColumnWidth(m_searchDockListHWND, 4, 80);		
	ListView_SetColumnWidth(m_searchDockListHWND, 5, 140);		
}

bool CProcessSearchInFiles::checkCancelButton() {
	MSG msg;
	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if(!::IsDialogMessage(m_searchInputDlgHnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);  
		}
	}

	if (m_searchDock->m_bStopPressed) {
		CUTL_BUFFER tempBuf;

		if (::MessageBox(m_searchInputDlgHnd, tempBuf.Sf("Stop the search at %s?", m_percentageProgress.GetSafe()), "Search in Files", MB_YESNO) == IDYES) {
			::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), TRUE);
			return true;
		}
	}

	// Enable Cancel button (if it was pressed)
	if(m_searchDock->m_bStopPressed) ::EnableWindow(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL), TRUE);
	m_searchDock->m_bStopPressed = false;
	::SetFocus(::GetDlgItem(m_searchInputDlgHnd, IDCANCEL));
	return false;
}

void CProcessSearchInFiles::ResizeInputDgl() {
	RECT	wndRect;
	POINT	coordinates;

	::GetWindowRect(m_searchInputDlgHnd, &wndRect);

	coordinates.x = wndRect.left;
	coordinates.y = wndRect.top;

	if (m_searchDock->m_bInProcess) {
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
	UINT pos;

	strFolder.Lower();   
   
	// We take this folder out of the search (05.10.07)
	if (strFolder.Find("jose\\wwwroot\\fotos", pos)) return true;

	if (strFolder[strFolder.Len() - 1] != '\\') strFolder += "\\";

	CUTL_PATH   tempPath(strFolder.GetSafe());
	CUTL_BUFFER nextFolder, msg;

	if (m_foldersArray.Len() == 0) 
		m_foldersArray = "#";
	else
		m_foldersArray += ",";

	m_foldersArray += folder;

	// Avisamos de los que estamos haciendo:
	::SetWindowText(m_searchDockStaticHWND, msg.Sf("Adding folder: '%s' ...", (LPCSTR)strFolder));

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
		if (m_bExcludeExtensions && m_excludeExtensionsList.Len()) {
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

		CUTL_BUFFER  StringFile, tempStringFile, StringLine, err;
		
		UINT      hitPos, startPos = 0, endPos = 0;

		// Let's read the file
		int fileSize = FileSize(file);

		if (!fileSize) return true;

		fp = fopen(file, "r");
	    
		if (!fp) return true;
		StringFile.Realloc(fileSize + 1);
		int lenFile = int(fread(StringFile, 1, fileSize, fp));
		fclose(fp);

		/*
		if (lenFile != fileSize) {
			systemMessageEx(err.Sf("Error reading file '%s'", file), __FILE__, __LINE__);
			return true;
		}
		*/
		StringFile[lenFile] = '\0';

		tempStringFile = StringFile;

		// Let's take care of the checks
		if (!m_bCaseSens) {
			tempStringFile.Upper();
			searchPattern.Upper();
		}
		if (m_bWholeWord) {
			tempBuf = searchPattern;
			searchPattern.Sf(" %s ", tempBuf.GetSafe());
		}

		while (tempStringFile.Find("\n", hitPos, endPos)) {
			if (checkCancelButton()) return false;

			if (hitPos - endPos > 110) {
				lineToShow.NCopy(&StringFile[endPos], 110);
				lineToShow += "...";
			}
			else
				lineToShow.NCopy(&StringFile[endPos], hitPos - endPos);
			if (!FindInLine(tempBuf.NCopy(&tempStringFile[endPos], hitPos - endPos), lineToShow.GetSafe(), searchPattern, iterator, line++))
				return false;

			endPos = hitPos + 1;
		}
      
		// Tratamos el resto del buffer.
		if (endPos < tempStringFile.Len()) {
			if (tempStringFile.Len() - endPos > 128) {
				lineToShow.NCopy(&StringFile[endPos], 128);
				lineToShow += "...";
			}
			else
				lineToShow.NCopy(&StringFile[endPos], tempStringFile.Len() - endPos);
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

bool CProcessSearchInFiles::FindInLine(LPCSTR strLine, LPCSTR lineToShow, LPCSTR searchPattern, CUTL_PATH iterator, int line) {
	try {
		SHFILEINFO  sfi;
		UINT        hitPos, size, found, endPosLine = 0;
		CUTL_BUFFER bufLine, nameExtension, driveDirectory, bufSize, statusText, bufFileMask;
		CUTL_BUFFER StringLine(strLine), formatDate;
		CCR3_DATE   date;

		while (StringLine.Find(searchPattern, hitPos, endPosLine)) {
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

			size = iterator.GetSize();
			iterator.GetTime(date);

			LVITEM listItem;

			memset(&listItem, 0, sizeof(LVITEM));

			listItem.mask = (m_searchDock->getCurrentSearchResultsDialog()->m_searchResultsListCtrl.hasImageList()) ? LVIF_IMAGE | LVIF_TEXT : LVIF_TEXT;
			listItem.cchTextMax = 256;

			listItem.iItem = ListView_GetItemCount(m_searchDockListHWND);
			listItem.iSubItem = 0;
			if (m_searchDock->getCurrentSearchResultsDialog()->m_searchResultsListCtrl.hasImageList()) {
				memset(&sfi, 0, sizeof(sfi));
				SHGetFileInfo ((LPCSTR)iterator, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), SHGFI_SMALLICON | SHGFI_SYSICONINDEX | SHGFI_USEFILEATTRIBUTES);
				listItem.iImage = sfi.iIcon;
			}

			listItem.pszText = (LPSTR)nameExtension.GetSafe();
			SendMessage(m_searchDockListHWND, LVM_SETITEMSTATE, listItem.iItem, (LPARAM)&listItem);
			SendMessage(m_searchDockListHWND, LVM_INSERTITEM, 0, (LPARAM)&listItem); // Send info to the Listview

			// Now the other columns
			// Text
			listItem.mask = LVIF_TEXT;
			listItem.iSubItem = 1;
			listItem.pszText = (LPSTR)bufLine.GetSafe();
			SendMessage(m_searchDockListHWND, LVM_SETITEM, 0,(LPARAM)&listItem);

			// Folder
			listItem.iSubItem = 2;
			listItem.pszText = (LPSTR)driveDirectory.GetSafe();
			SendMessage(m_searchDockListHWND, LVM_SETITEM, 0,(LPARAM)&listItem);

			// Line
			listItem.iSubItem = 3;
			listItem.pszText = bufSize.Sf("%12ld", line);
			SendMessage(m_searchDockListHWND, LVM_SETITEM, 0,(LPARAM)&listItem);

			// Hit pos
			listItem.iSubItem = 4;
			listItem.pszText = bufSize.Sf("%12ld", hitPos + 1);
			SendMessage(m_searchDockListHWND, LVM_SETITEM, 0,(LPARAM)&listItem);

			// Date Modified
			listItem.iSubItem = 5;
			date.Format(CCR3_DATE::DDMMYYHHMISS, &formatDate);
			listItem.pszText = (LPSTR)formatDate.GetSafe();
			SendMessage(m_searchDockListHWND, LVM_SETITEM, 0,(LPARAM)&listItem);

			m_totalHits++;
			endPosLine = hitPos + 1;

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
	if (m_foldersArray[m_foldersArray.Len() - 1] == ',')
		m_foldersArray[m_foldersArray.Len() - 1] = ';';
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
			::SetWindowText(::GetDlgItem(m_searchInputDlgHnd, IDC_STATIC_PROGRESS), m_percentageProgress.Sf("%.2lf%%", ((float)(100*i) / (float)foldersParse.NumArgs())));

			if (checkCancelButton()) return false;

			CUTL_PATH iterator;

			iterator.SetDriveDirectory(foldersParse.StrArg(i));
			if (!fileMaskEdit.Len()) fileMaskEdit = "*.*";
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
						::SetWindowText(m_searchDockStaticHWND, statusText.GetSafe());

						if (checkCancelButton()) return false;
						if (!FindInfile((LPCSTR)iterator)) return false;
						if (checkCancelButton()) return false;
					} while (iterator.FindNext());
				}
				if (checkCancelButton()) return false;
			}
		} 

		// If there is something on the list
		if (m_totalHits) {
			// Ask the list Control to adjust columns
			for (int i = 0; i < 6; i++)
				ListView_SetColumnWidth(m_searchDockListHWND, i, LVSCW_AUTOSIZE);		
		}

	}
	catch (...) {
		systemMessageEx("Error at CProcessSearchInFiles::SearchInFolders", __FILE__, __LINE__);
	}
	return true;
}