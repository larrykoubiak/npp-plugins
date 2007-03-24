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

#ifndef SEARCHINFILES_DLG_H
#define SEARCHINFILES_DLG_H

#include "../MISC/sysMsg/sysMsg.h"
#include "DockingDlgInterface.h"
#include "../TabBar/ControlsTab.h"
#include "StaticDialog.h"
#include "../WinControls/ImageListSet/ImageListSet.h"

#include "PluginInterface.h"

#define SCINTILLA_USER     (WM_USER + 2000)
#define WM_DOOPEN		   (SCINTILLA_USER + 8)

class SearchInFilesDock;
class searchResultsWindow;

typedef std::vector<searchResultsWindow*> SearchResultsDlgVector;

//////////////////////////////////////////////////////////////////////////////////////
// This dialog retrieves the search in files options
class SearchInputDlg : public StaticDialog
{
	public:
		static BOOL CALLBACK SearchInFilesInputDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
		static BOOL CALLBACK SearchInFilesExcludeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
};

//////////////////////////////////////////////////////////////////////////////////////
// This class manages the dockable search in files interface
class SearchInFilesDock : public DockingDlgInterface
{
public :
	SearchInFilesDock() : DockingDlgInterface(IDD_SEARCHINFILESDOCK), m_bShowSearchInFiles(false) {};

	char m_iniFilePath[MAX_PATH];
	bool m_bShowSearchInFiles;
	
	LPSTR getSectionName() { return "Search in Files Extension"; };
	LPSTR getKeyName() { return "showSearchInFiles"; };

	void chooseFolder(HWND hDlg);
	void callSearchInFiles(HWND hDlg, CUTL_BUFFER what, CUTL_BUFFER types, CUTL_BUFFER where);

    virtual void display(bool toShow = true) const { DockingDlgInterface::display(toShow); };
	void setParent(HWND parent2set){ _hParent = parent2set; };

	void openSearchInFilesInputDlg();
	static BOOL CALLBACK SearchInFilesInputDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bool	m_bInProcess;
	bool	m_bStopPressed;
	HWND	m_nppHandle;
	HWND	m_scintillaMainHandle;

	searchResultsWindow* getCurrentSearchResultsDialog() { return m_searchResultsDlgVector[_ctrlTab.getCurrentTab()]; };

	void moveToNextHit();

	void SaveCombosStrings(HWND hDlg);
	void LoadCombosStrings(HWND hDlg);

	void SaveChecks(HWND hDlg);
	void LoadChecks(HWND hDlg);

protected :
	SIFControlsTab			_ctrlTab;
	SIFIconList				_ctrlTabIconList;
	searchResultsWindow		_searchResultsDlg;

	SearchResultsDlgVector	m_searchResultsDlgVector;

	WindowVector			_wVector;

	virtual BOOL CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};

#endif //SEARCHINFILES_DLG_H